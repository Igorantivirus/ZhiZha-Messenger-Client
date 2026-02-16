#pragma once

#include <chrono>
#include <cstddef>

namespace ws
{

struct ClientOptions
{
    std::size_t readMessageMaxBytes = 8 * 1024 * 1024;
    std::size_t maxSendQueueBytes = 8 * 1024 * 1024;
    std::chrono::seconds connectTimeout = std::chrono::seconds(15);
    std::chrono::seconds handshakeTimeout = std::chrono::seconds(15);
    std::chrono::seconds readTimeout = std::chrono::seconds(0);
    std::chrono::seconds writeTimeout = std::chrono::seconds(0);
    bool queueBeforeOpen = true;
};

} // namespace ws
