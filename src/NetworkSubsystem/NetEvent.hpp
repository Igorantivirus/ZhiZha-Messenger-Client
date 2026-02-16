#pragma once

#include <boost/beast/websocket/rfc6455.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

class NetEvent
{
public:
    enum class Type
    {
        onOpen,
        onClose,
        onText,
        onBytes,
        onError
    };
    struct OnOpen
    {
    };
    struct OnText
    {
        std::string text;
    };
    struct OnBytes
    {
        std::vector<std::uint8_t> bytes;
    };
    struct OnClose
    {
        boost::beast::websocket::close_reason reason;
    };
    struct OnError
    {
        std::string stage;
        boost::beast::error_code ec;
    };

    template <typename T>
    std::optional<T> get()
    {
        if (auto *val = std::get_if<T>(&data_))
            return *val;
        return std::nullopt;
    }

    const Type getType()
    {
        return type_;
    }

    using DataValue = std::variant<OnOpen, OnText, OnBytes, OnClose, OnError>;

    void setData(DataValue data, const Type t)
    {
        data_ = std::move(data);
        type_ = t;
    }

private:
    

    Type type_;
    DataValue data_;
};