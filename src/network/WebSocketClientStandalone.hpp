#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "Utils/ClientOptions.hpp"
#include "Utils/NetworkEvents.hpp"
#include "Utils/WsUrl.hpp"
#include "Utils/State.hpp"

namespace ws
{
namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

class WebSocketSession;

class WebSocketClientStandalone : public std::enable_shared_from_this<WebSocketClientStandalone>
{
public:

    static std::shared_ptr<WebSocketClientStandalone> create();

    ~WebSocketClientStandalone();

    void setOptions(ClientOptions options);
    void setCallbackExecutor(net::any_io_executor ex);

    NetworkEvents& events();
    const NetworkEvents& events() const;

    void start(WsUrl url);

    void sendText(std::string msg);
    void sendBytes(std::span<const std::uint8_t> bytes);

    void stop(websocket::close_code code = websocket::close_code::normal, std::string reason = {});
    void stopNow();
    [[nodiscard]] State getState() const noexcept;

private:
    WebSocketClientStandalone();

    void snapshotForSession(NetworkEvents& out_events, ClientOptions& out_options, net::any_io_executor& out_callback_ex);
    void decorateEventsForState(NetworkEvents& events);
    void startImpl(WsUrl url);

private:
    std::unique_ptr<net::io_context> ioc_;
    net::any_io_executor ex_;
    net::strand<net::any_io_executor> client_strand_;
    std::shared_ptr<WebSocketSession> session_;
    std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_;
    std::thread thread_;

    std::mutex cb_and_opt_mx_;
    NetworkEvents events_;
    ClientOptions options_;
    std::optional<net::any_io_executor> callback_ex_;
    std::atomic<State> state_ = State::idle;
};
} // namespace ws
