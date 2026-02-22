#pragma once

#include <cassert>
#include <string>
#include <utility>

#include <App/NetworkSubsystem/NetEvent.hpp>
#include <App/NetworkSubsystem/NetworkSubsystem.hpp>

#include <network/Utils/WsUrl.hpp>

#include "AppState.hpp"
#include "Events/AppEvent.hpp"
#include "Events/AppEventHub.hpp"
#include "KeyGenerator.hpp"
#include "protocol/JsonMessages.hpp"
#include "protocol/JsonPacker.hpp"
#include "protocol/JsonParser.hpp"

class RegistrationService
{
public:
    void wire(AppState &state, NetworkSubsystem &net, AppEventHub &events)
    {
        state_ = &state;
        net_ = &net;
        events_ = &events;
    }

    void connectAndRegister(std::string serverKey, std::string username, std::string password)
    {
        assert(state_ && net_ && events_ && "RegistrationService is not wired");

        pendingUsername_ = std::move(username);
        pendingPassword_ = std::move(password);
        state_->userName = pendingUsername_;

        ws::WsUrl url = KeyGenerator::fromKey(serverKey);
        net_->getClient()->start(std::move(url));
    }

    void onNetEvent(const NetEvent &event)
    {
        if (!state_ || !net_ || !events_)
            return;

        if (event.getType() == NetEvent::Type::onOpen)
        {
            if (pendingUsername_.empty())
                return;

            ClientRegisterRequest reg;
            reg.clientVersion = "1.0";
            reg.publicKey = "ABOBA";
            reg.username = pendingUsername_;
            reg.password = pendingPassword_;
            net_->getClient()->sendText(JsonPacker::packRegisterRequest(reg));
            return;
        }

        if (event.getType() == NetEvent::Type::onError)
        {
            const auto *e = event.getIf<NetEvent::OnError>();
            if (!e)
                return;
            AppEvent ev;
            ev.setData(AppEvent::RegisterFailed{.code = e->ec.to_string(), .message = e->stage}, AppEvent::Type::RegisterFailed);
            events_->dispatch(ev); 

        }

        if (event.getType() != NetEvent::Type::onText)
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

        if(typeopt.value() == "register-error")
        {
            auto error = JsonParser::parseServerErrorPayload(*jsonPayload);
            if(!error.has_value())
                return;
            AppEvent ev;
            ev.setData(AppEvent::RegisterFailed{.code = error->code, .message = error->message}, AppEvent::Type::RegisterFailed);
            events_->dispatch(ev);    
        }

        if (typeopt.value() != "register-result")
            return;

        auto request = JsonParser::parseServerRegistrationPayload(*jsonPayload);
        if (!request)
            return;

        state_->userID = request->userId;

        AppEvent ev;
        ev.setData(AppEvent::RegisterSucceeded{.userId = request->userId, .userName = state_->userName},
                   AppEvent::Type::RegisterSucceeded);
        events_->dispatch(ev);
    }

private:
    AppState *state_ = nullptr;
    NetworkSubsystem *net_ = nullptr;
    AppEventHub *events_ = nullptr;
    std::string pendingUsername_;
    std::string pendingPassword_;
};

