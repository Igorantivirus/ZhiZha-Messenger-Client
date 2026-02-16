#include "WebSocketClient.hpp"

#include <utility>
#include <vector>
#include <string_view>

#include "Utils/WebSocketSession.hpp"

namespace ws
{
std::shared_ptr<WebSocketClient> WebSocketClient::create(net::io_context& ioc)
{
    return std::shared_ptr<WebSocketClient>(new WebSocketClient(ioc.get_executor()));
}

WebSocketClient::~WebSocketClient()
{
    if (session_)
    {
        auto session = std::move(session_);
        session->closeNow();
    }
}

void WebSocketClient::setOptions(ClientOptions options)
{
    std::scoped_lock lock(cb_and_opt_mx_);
    options_ = std::move(options);
}

void WebSocketClient::setCallbackExecutor(net::any_io_executor ex)
{
    std::scoped_lock lock(cb_and_opt_mx_);
    callback_ex_ = std::move(ex);
}

NetworkEvents& WebSocketClient::events()
{
    return events_;
}

const NetworkEvents& WebSocketClient::events() const
{
    return events_;
}

void WebSocketClient::start(WsUrl url)
{
    net::post(client_strand_,
        [self = shared_from_this(), url = std::move(url)]() mutable {
            self->startImpl(std::move(url));
        });
}

void WebSocketClient::sendText(std::string msg)
{
    net::post(client_strand_, [self = shared_from_this(), msg = std::move(msg)]() mutable {
        if (self->session_)
            self->session_->sendText(std::move(msg));
    });
}

void WebSocketClient::sendBytes(std::span<const std::uint8_t> bytes)
{
    std::vector<std::uint8_t> copy(bytes.begin(), bytes.end());
    net::post(client_strand_, [self = shared_from_this(), copy = std::move(copy)]() mutable {
        if (self->session_)
            self->session_->sendBytes(std::move(copy));
    });
}

void WebSocketClient::stop(websocket::close_code code, std::string reason)
{
    websocket::close_reason close_reason{code};
    close_reason.reason = std::move(reason);

    net::post(client_strand_, [self = shared_from_this(), close_reason]() mutable {
        self->state_.store(State::closing, std::memory_order_relaxed);
        if (self->session_)
        {
            self->session_->close(close_reason);
            self->session_.reset();
        }
        else
        {
            self->state_.store(State::closed, std::memory_order_relaxed);
        }
    });
}

void WebSocketClient::stopNow()
{
    net::post(client_strand_, [self = shared_from_this()]() mutable {
        self->state_.store(State::closing, std::memory_order_relaxed);
        if (self->session_)
        {
            self->session_->closeNow();
            self->session_.reset();
        }
        self->state_.store(State::closed, std::memory_order_relaxed);
    });
}

ws::State WebSocketClient::getState() const noexcept
{
    return state_.load(std::memory_order_relaxed);
}

WebSocketClient::WebSocketClient(net::any_io_executor ex)
    : ex_(std::move(ex))
    , client_strand_(net::make_strand(ex_))
{}

void WebSocketClient::snapshotForSession(NetworkEvents& out_events, ClientOptions& out_options, net::any_io_executor& out_callback_ex)
{
    std::scoped_lock lock(cb_and_opt_mx_);
    out_events = events_;
    out_options = options_;
    out_callback_ex = callback_ex_.value_or(ex_);
}

void WebSocketClient::startImpl(WsUrl url)
{
    if (session_)
    {
        state_.store(State::closing, std::memory_order_relaxed);
        session_->close(websocket::close_reason{websocket::close_code::normal});
        session_.reset();
    }

    NetworkEvents events;
    ClientOptions options;
    net::any_io_executor callback_ex;
    snapshotForSession(events, options, callback_ex);
    decorateEventsForState(events);

    state_.store(State::connecting, std::memory_order_relaxed);
    session_ = std::make_shared<WebSocketSession>(ex_, std::move(events), std::move(options), std::move(callback_ex));
    session_->run(std::move(url.host), std::move(url.port), std::move(url.target));
}

void WebSocketClient::decorateEventsForState(NetworkEvents& events)
{
    auto weak_self = weak_from_this();

    auto on_open = std::move(events.on_open);
    events.on_open = [weak_self, on_open = std::move(on_open)]() mutable {
        if (auto self = weak_self.lock())
            self->state_.store(State::open, std::memory_order_relaxed);
        if (on_open)
            on_open();
    };

    auto on_close = std::move(events.on_close);
    events.on_close = [weak_self, on_close = std::move(on_close)](websocket::close_reason reason) mutable {
        if (auto self = weak_self.lock())
            self->state_.store(State::closed, std::memory_order_relaxed);
        if (on_close)
            on_close(reason);
    };

    auto on_error = std::move(events.on_error);
    events.on_error = [weak_self, on_error = std::move(on_error)](std::string_view stage, beast::error_code ec) mutable {
        if (auto self = weak_self.lock())
            self->state_.store(State::error, std::memory_order_relaxed);
        if (on_error)
            on_error(stage, ec);
    };
}
} // namespace ws
