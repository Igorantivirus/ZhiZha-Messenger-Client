#pragma once

#include <string>
#include <variant>

#include <Core/Types.hpp>

class AppEvent
{
public:
    enum class Type
    {
        RegisterSucceeded,
        RegisterFailed,
        ChatMessageReceived,
        ConnectionClosed,
        ConnectionError,
    };

    struct RegisterSucceeded
    {
        IDType userId = 0;
        std::string userName;
    };

    struct RegisterFailed
    {
        std::string code;
        std::string message;
    };

    struct ChatMessageReceived
    {
        std::string userName;
        std::string message;
    };

    struct ConnectionClosed
    {
        std::string reason;
    };

    struct ConnectionError
    {
        std::string stage;
        std::string message;
    };

    using DataValue = std::variant<RegisterSucceeded, RegisterFailed, ChatMessageReceived, ConnectionClosed, ConnectionError>;

    template <typename T>
    const T *getIf() const
    {
        return std::get_if<T>(&data_);
    }

    Type getType() const
    {
        return type_;
    }

    void setData(DataValue data, const Type t)
    {
        data_ = std::move(data);
        type_ = t;
    }

private:
    Type type_ = Type::ConnectionError;
    DataValue data_{ConnectionError{}};
};

