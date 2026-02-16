#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "ClientOptions.hpp"
#include "NetworkEvents.hpp"
#include "OutMsg.hpp"
#include "State.hpp"

namespace ws
{
namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
public:
    WebSocketSession(net::any_io_executor ex, NetworkEvents events, ClientOptions options, net::any_io_executor callback_ex);

    void run(std::string host, std::string port, std::string target);
    void sendText(std::string msg);
    void sendBytes(std::vector<std::uint8_t> bytes);
    void close(websocket::close_reason reason);
    void closeNow();

private:

    void emitOpen();
    void emitText(std::string msg);
    void emitBytes(std::vector<std::uint8_t> data);
    void emitError(std::string_view stage, beast::error_code ec);
    void emitCloseOnce(websocket::close_reason reason);

    void doResolve();
    void onResolve(beast::error_code ec, tcp::resolver::results_type results);
    void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void onHandshake(beast::error_code ec);

    void doRead();
    void onRead(beast::error_code ec, std::size_t bytes);

    void enqueueText(std::string msg);
    void enqueueBytes(std::vector<std::uint8_t> bytes);
    bool pushOutgoing(OutMsg msg);
    void doWriteIfNeeded();
    void onWrite(beast::error_code ec, std::size_t bytes);

    void forceClose();
    void startClose(websocket::close_reason reason);
    void onClose(beast::error_code ec);
    void fail(std::string_view stage, beast::error_code ec);

private:
    net::strand<net::any_io_executor> strand_;
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer read_buf_;

    NetworkEvents events_;
    ClientOptions options_;
    net::any_io_executor callback_ex_;

    std::string host_;
    std::string port_;
    std::string target_;

    std::deque<OutMsg> outq_;
    std::size_t outq_bytes_ = 0;
    bool write_in_progress_ = false;

    State state_ = State::idle;
    bool close_emitted_ = false;
    bool stop_requested_ = false;
};
} // namespace ws
