#pragma once

#include "network/Utils/WsUrl.hpp"
#include <string>

class KeyGenerator
{
public:

    static std::string generateKey(const std::string &ip, const int port)
    {
        return ip + ':' + std::to_string(port) + "/ws";
    }

    static ws::WsUrl fromKey(const std::string &key)
    {
        ws::WsUrl res;
        std::size_t dd = key.find(':');
        std::size_t slesh = key.find('/');

        if (dd == std::string::npos || slesh == std::string::npos || slesh == 0 || dd == key.size() - 1)
            return res;
        res.host = key.substr(0, dd);
        res.target = key.substr(slesh);
        res.port = key.substr(dd + 1, slesh - dd - 1);
        return res;
    }
};