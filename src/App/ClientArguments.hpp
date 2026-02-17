#pragma once

#include <cassert>

#include <Engine/Arguments.hpp>
#include <App/NetworkSubsystem/NetworkSubsystem.hpp>
#include <App/NetworkSubsystem/NetEventHub.hpp>

#include "AppContext.hpp"

struct ClientArguments : public engine::Arguments
{
    NetworkSubsystem *network = nullptr;
    NetEventHub *netHub = nullptr;
    AppContext *app = nullptr;

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

    AppContext &appContext() const
    {
        assert(app && "ClientArguments::app is null");
        return *app;
    }

    AppState &appState() const
    {
        return appContext().state();
    }
};
