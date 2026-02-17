#include "WebSocketSession.hpp"

#include <utility>

namespace ws
{
WebSocketSession::WebSocketSession(net::any_io_executor ex, NetworkEvents events, ClientOptions options, net::any_io_executor callback_ex)
    : strand_(net::make_strand(ex))
    , resolver_(strand_)
    , ws_(strand_)
    , events_(std::move(events))
    , options_(std::move(options))
    , callback_ex_(std::move(callback_ex))
{
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    ws_.read_message_max(options_.readMessageMaxBytes);
    ws_.auto_fragment(true);
    ws_.set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
        req.set(beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " ws-facade-client");
    }));
}

void WebSocketSession::run(std::string host, std::string port, std::string target)
{
    net::post(strand_, [self = shared_from_this(), host = std::move(host), port = std::move(port), target = std::move(target)]() mutable {
        self->host_ = std::move(host);
        self->port_ = std::move(port);
        self->target_ = std::move(target);
        self->doResolve();
    });
}

void WebSocketSession::sendText(std::string msg)
{
    net::post(strand_, [self = shared_from_this(), msg = std::move(msg)]() mutable {
        self->enqueueText(std::move(msg));
    });
}

void WebSocketSession::sendBytes(std::vector<std::uint8_t> bytes)
{
    net::post(strand_, [self = shared_from_this(), bytes = std::move(bytes)]() mutable {
        self->enqueueBytes(std::move(bytes));
    });
}

void WebSocketSession::close(websocket::close_reason reason)
{
    net::post(strand_, [self = shared_from_this(), reason]() mutable {
        self->startClose(reason);
    });
}

void WebSocketSession::closeNow()
{
    net::post(strand_, [self = shared_from_this()]() mutable {
        self->forceClose();
    });
}

void WebSocketSession::emitOpen()
{
    if (!events_.on_open)
        return;
    auto cb = events_.on_open;
    net::post(callback_ex_, [cb = std::move(cb)]() mutable { cb(); });
}

void WebSocketSession::emitText(std::string msg)
{
    if (!events_.on_text)
        return;
    auto cb = events_.on_text;
    net::post(callback_ex_, [cb = std::move(cb), msg = std::move(msg)]() mutable { cb(std::move(msg)); });
}

void WebSocketSession::emitBytes(std::vector<std::uint8_t> data)
{
    if (!events_.on_bytes)
        return;
    auto cb = events_.on_bytes;
    net::post(callback_ex_, [cb = std::move(cb), data = std::move(data)]() mutable {
        cb(std::move(data));
    });
}

void WebSocketSession::emitError(std::string_view stage, beast::error_code ec)
{
    if (!events_.on_error)
        return;
    auto cb = events_.on_error;
    net::post(callback_ex_, [cb = std::move(cb), stage = std::string(stage), ec]() mutable { cb(stage, ec); });
}

void WebSocketSession::emitCloseOnce(websocket::close_reason reason)
{
    if (close_emitted_)
        return;
    close_emitted_ = true;

    if (!events_.on_close)
        return;
    auto cb = events_.on_close;
    net::post(callback_ex_, [cb = std::move(cb), reason]() mutable { cb(reason); });
}

void WebSocketSession::doResolve()
{
    if (state_ == State::closing || state_ == State::closed)
        return;

    state_ = ws::State::resolving;
    resolver_.async_resolve(host_, port_, beast::bind_front_handler(&WebSocketSession::onResolve, shared_from_this()));
}

void WebSocketSession::onResolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec)
        return fail("resolve", ec);

    if (stop_requested_)
        return startClose(websocket::close_reason{websocket::close_code::normal});

    state_ = State::connecting;
    beast::get_lowest_layer(ws_).expires_after(options_.connectTimeout);
    beast::get_lowest_layer(ws_).async_connect(results, beast::bind_front_handler(&WebSocketSession::onConnect, shared_from_this()));
}

void WebSocketSession::onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
{
    if (ec)
        return fail("connect", ec);

    if (stop_requested_)
        return startClose(websocket::close_reason{websocket::close_code::normal});

    state_ = ws::State::handshaking;
    beast::get_lowest_layer(ws_).expires_after(options_.handshakeTimeout);
    ws_.async_handshake(host_, target_, beast::bind_front_handler(&WebSocketSession::onHandshake, shared_from_this()));
}

void WebSocketSession::onHandshake(beast::error_code ec)
{
    if (ec)
        return fail("handshake", ec);

    beast::get_lowest_layer(ws_).expires_never();
    state_ = State::open;
    emitOpen();

    doRead();
    doWriteIfNeeded();
}

void WebSocketSession::doRead()
{
    if (state_ != State::open)
        return;

    if (options_.readTimeout.count() > 0)
        beast::get_lowest_layer(ws_).expires_after(options_.readTimeout);
    else
        beast::get_lowest_layer(ws_).expires_never();

    ws_.async_read(read_buf_, beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this()));
}

void WebSocketSession::onRead(beast::error_code ec, std::size_t)
{
    if (ec == websocket::error::closed)
    {
        state_ = State::closed;
        emitCloseOnce(ws_.reason());
        return;
    }

    if (ec)
        return fail("read", ec);

    if (ws_.got_text())
    {
        std::string text = beast::buffers_to_string(read_buf_.data());
        read_buf_.consume(read_buf_.size());
        emitText(std::move(text));
    }
    else
    {
        auto data = read_buf_.data();
        std::vector<std::uint8_t> bytes(beast::buffer_bytes(data));
        net::buffer_copy(net::buffer(bytes), data);
        read_buf_.consume(read_buf_.size());
        emitBytes(std::move(bytes));
    }

    doRead();
}

void WebSocketSession::enqueueText(std::string msg)
{
    if (state_ == State::closing || state_ == State::closed)
        return;

    if (state_ != State::open && !options_.queueBeforeOpen)
    {
        emitError("send_before_open", beast::error_code{net::error::not_connected});
        return;
    }

    if (!pushOutgoing(OutMsg{std::move(msg)}))
        return;

    doWriteIfNeeded();
}

void WebSocketSession::enqueueBytes(std::vector<std::uint8_t> bytes)
{
    if (state_ == State::closing || state_ == State::closed)
        return;

    if (state_ != State::open && !options_.queueBeforeOpen)
    {
        emitError("send_before_open", beast::error_code{net::error::not_connected});
        return;
    }

    if (!pushOutgoing(OutMsg{std::move(bytes)}))
        return;

    doWriteIfNeeded();
}

bool WebSocketSession::pushOutgoing(OutMsg msg)
{
    const std::size_t size = msg.size();
    if (size == 0)
        return true;

    if (outq_bytes_ + size > options_.maxSendQueueBytes)
    {
        emitError("send_queue_overflow", beast::error_code{net::error::no_buffer_space});
        return false;
    }

    outq_bytes_ += size;
    outq_.push_back(std::move(msg));
    return true;
}

void WebSocketSession::doWriteIfNeeded()
{
    if (state_ != State::open || write_in_progress_ || outq_.empty())
        return;

    write_in_progress_ = true;

    if (options_.writeTimeout.count() > 0)
        beast::get_lowest_layer(ws_).expires_after(options_.writeTimeout);
    else
        beast::get_lowest_layer(ws_).expires_never();

    const auto& front = outq_.front();
    ws_.text(front.isText());

    std::visit(
        [self = shared_from_this()](const auto& payload) {
            self->ws_.async_write(net::buffer(payload), beast::bind_front_handler(&WebSocketSession::onWrite, self));
        },
        front.payload);
}

void WebSocketSession::onWrite(beast::error_code ec, std::size_t)
{
    if (ec == websocket::error::closed)
    {
        state_ = State::closed;
        emitCloseOnce(ws_.reason());
        return;
    }

    if (ec)
        return fail("write", ec);

    if (!outq_.empty())
    {
        outq_bytes_ -= outq_.front().size();
        outq_.pop_front();
    }

    write_in_progress_ = false;
    doWriteIfNeeded();
}

void WebSocketSession::forceClose()
{
    if (state_ == State::closed)
        return;

    stop_requested_ = true;
    resolver_.cancel();

    beast::error_code ignored;
    beast::get_lowest_layer(ws_).socket().cancel(ignored);
    beast::get_lowest_layer(ws_).socket().close(ignored);

    state_ = State::closed;
    emitCloseOnce(websocket::close_reason{websocket::close_code::normal});
}

void WebSocketSession::startClose(websocket::close_reason reason)
{
    if (state_ == State::closed)
    {
        emitCloseOnce(reason);
        return;
    }

    stop_requested_ = true;

    if (state_ != State::open)
    {
        resolver_.cancel();
        beast::error_code ignored;
        beast::get_lowest_layer(ws_).socket().cancel(ignored);
        beast::get_lowest_layer(ws_).socket().close(ignored);
        state_ = State::closed;
        emitCloseOnce(reason);
        return;
    }

    if (state_ == State::closing)
        return;

    state_ = State::closing;
    ws_.async_close(reason, beast::bind_front_handler(&WebSocketSession::onClose, shared_from_this()));
}

void WebSocketSession::onClose(beast::error_code ec)
{
    state_ = State::closed;

    if (ec && ec != websocket::error::closed && ec != net::error::operation_aborted)
        emitError("close", ec);

    emitCloseOnce(ws_.reason());
}

void WebSocketSession::fail(std::string_view stage, beast::error_code ec)
{
    if (ec == net::error::operation_aborted)
    {
        startClose(websocket::close_reason{websocket::close_code::normal});
        return;
    }

    emitError(stage, ec);
    startClose(websocket::close_reason{websocket::close_code::normal});
}
} // namespace ws
