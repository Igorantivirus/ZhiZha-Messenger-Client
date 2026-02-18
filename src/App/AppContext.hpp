#pragma once

#include <cassert>

#include "AppState.hpp"

#include <App/NetworkSubsystem/INetEventListener.hpp>
#include <App/NetworkSubsystem/NetworkSubsystem.hpp>

#include "Events/AppEventHub.hpp"
#include "Services/ChatService.hpp"
#include "Services/RegistrationService.hpp"

// App-wide services/state container.
// Kept separate from engine::Arguments to avoid mutable global state inside const arguments.
class AppContext
{
public:
    void bindNetwork(NetworkSubsystem &network)
    {
        network_ = &network;
        registration_.wire(state_, network, events_);
        chat_.wire(state_, network, events_);
    }

    NetworkSubsystem &net() const
    {
        assert(network_ && "AppContext::network is null (bindNetwork was not called)");
        return *network_;
    }

    const AppState &state() const noexcept
    {
        return state_;
    }

    AppState &state() noexcept
    {
        return state_;
    }

    RegistrationService &registration() noexcept
    {
        return registration_;
    }

    ChatService &chat() noexcept
    {
        return chat_;
    }

    AppEventHub &events() noexcept
    {
        return events_;
    }

    INetEventListener &netListener() noexcept
    {
        return netListener_;
    }

private:
    void onNetEvent_(const NetEvent &ev)
    {
        registration_.onNetEvent(ev);
        chat_.onNetEvent(ev);
    }

private:
    class NetListener final : public INetEventListener
    {
    public:
        explicit NetListener(AppContext &ctx) : ctx_(ctx) {}

        void onNetEvent(const NetEvent &ev) override
        {
            ctx_.onNetEvent_(ev);
        }

    private:
        AppContext &ctx_;
    };

private:
    NetworkSubsystem *network_ = nullptr;
    AppState state_{};
    AppEventHub events_{};

    RegistrationService registration_{};
    ChatService chat_{};

    NetListener netListener_{*this};
};
