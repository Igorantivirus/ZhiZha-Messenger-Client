#pragma once

#include <cassert>

#include <Engine/Arguments.hpp>
#include <NetworkSubsystem/NetworkSubsystem.hpp>
#include <NetworkSubsystem/NetEventHub.hpp>
#include "AppState.hpp"

struct ClientArguments : public engine::Arguments
{
    NetworkSubsystem *network = nullptr;
    NetEventHub *netHub = nullptr;
    mutable AppState appstate;

    NetworkSubsystem &net() const
    {
        assert(network && "ClientArguments::network is null");
        return *network;
    }

    NetEventHub &netEvents() const
    {
        assert(netHub && "ClientArguments::netHub is null");
        return *netHub;
    }

    AppState& appState() const
    {
        return appstate;
    }
};
