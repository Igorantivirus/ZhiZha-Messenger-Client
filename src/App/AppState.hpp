#pragma once

#include <string>
#include <map>

#include "Core/Types.hpp"
struct AppState
{
public:

    IDType userID = 0;
    std::string userName;
    std::map<IDType, std::string> chats;
    std::map<IDType, std::string> users;

};