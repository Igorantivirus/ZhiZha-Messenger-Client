#pragma once

#include <cassert>
#include <string>

#include <App/NetworkSubsystem/NetEvent.hpp>
#include <App/NetworkSubsystem/NetworkSubsystem.hpp>

#include "AppState.hpp"
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

    void sendMessage(const std::string &text, const int chatId = 1)
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

        if (typeopt.value() != "chat-msg")
            return;

        auto request = JsonParser::parseServerChatMessagePayload(*jsonPayload);
        if (!request)
            return;

        AppEvent ev;
        ev.setData(AppEvent::ChatMessageReceived{.userName = request->userName, .message = request->message},
                   AppEvent::Type::ChatMessageReceived);
        events_->dispatch(ev);
    }

private:
    AppState *state_ = nullptr;
    NetworkSubsystem *net_ = nullptr;
    AppEventHub *events_ = nullptr;
};

