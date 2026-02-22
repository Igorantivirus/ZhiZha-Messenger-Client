#pragma once

#include <cassert>
#include <string>

#include <App/NetworkSubsystem/NetEvent.hpp>
#include <App/NetworkSubsystem/NetworkSubsystem.hpp>

#include "AppState.hpp"
#include "Core/Types.hpp"
#include "Events/AppEvent.hpp"
#include "Events/AppEventHub.hpp"
#include "protocol/JsonMessages.hpp"
#include "protocol/JsonPacker.hpp"
#include "protocol/JsonParser.hpp"

class ChatService
{
public:
    void wire(AppState &state, NetworkSubsystem &net, AppEventHub &events)
    {
        state_ = &state;
        net_ = &net;
        events_ = &events;
    }

    void sendMessage(const std::string &text, const IDType chatId)
    {
        assert(state_ && net_ && "ChatService is not wired");

        ClientChatMessageRequest mr;
        mr.chatId = chatId;
        mr.userId = state_->userID;
        mr.message = text;
        if (mr.message.empty())
            return;

        net_->getClient()->sendText(JsonPacker::packChatMessageRequest(mr));
    }

    void sendChatsRequest()
    {
        ClientDataRequest req;
        req.dataType = "chats";
        req.userId = state_->userID;
        net_->getClient()->sendText(JsonPacker::packDataRequest(req));
    }

    void sendUsersRequest()
    {
        ClientDataRequest req;
        req.dataType = "users";
        req.userId = state_->userID;
        net_->getClient()->sendText(JsonPacker::packDataRequest(req));
    }

    void sendCreateRoomRequest(bool isPrivate, std::vector<IDType> users, std::string name)
    {
        ClientCreateRoomRequest req;
        req.isPrivate = isPrivate;
        req.userId = state_->userID;
        req.participantUserIds = std::move(users);
        req.name = std::move(name);
        net_->getClient()->sendText(JsonPacker::packCreateRoomRequest(req));
    }

    void onNetEvent(const NetEvent &event)
    {
        if (!events_ || event.getType() != NetEvent::Type::onText)
            return;

        const auto *t = event.getIf<NetEvent::OnText>();
        if (!t)
            return;

        const auto jsonPayload = JsonParser::parseJson(t->text);
        if (!jsonPayload.has_value())
            return;

        const auto typeopt = JsonParser::parseMessageType(*jsonPayload);
        if (!typeopt.has_value())
            return;

        if (typeopt.value() == "chat-msg")
        {
            auto request = JsonParser::parseServerChatMessagePayload(*jsonPayload);
            if (!request)
                return;
            AppEvent ev;
            ev.setData(AppEvent::ChatMessageReceived{.chatID = request->chatId, .userName = request->userName, .message = request->message}, AppEvent::Type::ChatMessageReceived);
            events_->dispatch(ev);
        }
        else if (typeopt.value() == "chats-payload")
        {
            auto request = JsonParser::parseServerChatsRequestPayload(*jsonPayload);
            if (!request)
                return;

            state_->chats = request->chats;

            AppEvent ev;
            ev.setData(AppEvent::ChatsPayload{}, AppEvent::Type::ChatsPayload);
            events_->dispatch(ev);
        }
        else if (typeopt.value() == "users-payload")
        {
            auto request = JsonParser::parseServerUsersRequestPayload(*jsonPayload);
            if (!request)
                return;

            state_->users = request->users;

            AppEvent ev;
            ev.setData(AppEvent::UsersPayload{}, AppEvent::Type::UsersPayload);
            events_->dispatch(ev);
        }
        else if (typeopt.value() == "user-change")
        {
            auto request = JsonParser::parseServerUsersSomeChange(*jsonPayload);
            if (!request)
                return;

            if(request->changeType == "registered")
            {
                state_->users[request->userId] = request->username;
            }
            if(request->changeType == "logout")
            {
                state_->users.erase(request->userId);
            }
            if(request->changeType == "rename")
            {
                state_->users[request->userId] = request->username;
            }
        }
        else if(typeopt.value() == "room-created")
        {
            auto request = JsonParser::parseServerRoomCreatedPayload(*jsonPayload);
            if (!request || !request->created)
                return;

            AppEvent ev;
            ev.setData(AppEvent::RoomCreated{.chatID = request->chatId, .users = std::move(request->participantUserIds), .name = request->name}, AppEvent::Type::RoomCreated);
            events_->dispatch(ev);
        }
    }

private:
    AppState *state_ = nullptr;
    NetworkSubsystem *net_ = nullptr;
    AppEventHub *events_ = nullptr;
};
