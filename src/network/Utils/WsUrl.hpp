#pragma once

#include <string>

namespace ws
{
struct WsUrl
{
    std::string host;
    std::string port;
    std::string target;
};
} // namespace ws
