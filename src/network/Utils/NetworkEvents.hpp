#pragma once

#include <cstdint>
#include <functional>
#include <utility>

#include <boost/beast/core/error.hpp>
#include <boost/beast/websocket/rfc6455.hpp>

namespace ws
{
class WebSocketSession;
class WebSocketClient;
class WebSocketClientStandalone;

class NetworkEvents
{
public:
    using OnOpen = std::function<void()>;
    using OnText = std::function<void(std::string)>;
    using OnBytes = std::function<void(std::vector<std::uint8_t>)>;
    using OnClose = std::function<void(boost::beast::websocket::close_reason)>;
    using OnError = std::function<void(std::string_view stage, boost::beast::error_code ec)>;

    void onOpen(OnOpen cb) { on_open = std::move(cb); }
    void onText(OnText cb) { on_text = std::move(cb); }
    void onBytes(OnBytes cb) { on_bytes = std::move(cb); }
    void onClose(OnClose cb) { on_close = std::move(cb); }
    void onError(OnError cb) { on_error = std::move(cb); }

private:
    OnOpen on_open;
    OnText on_text;
    OnBytes on_bytes;
    OnClose on_close;
    OnError on_error;

    friend class WebSocketSession;
    friend class WebSocketClient;
    friend class WebSocketClientStandalone;
};
} // namespace ws
