#pragma once

#include <cassert>

#include <Engine/Arguments.hpp>
#include <NetworkSubsystem/NetworkSubsystem.hpp>
#include "AppState.hpp"

struct ClientArguments : public engine::Arguments
{
    NetworkSubsystem *network = nullptr;
    mutable AppState appstate;

    NetworkSubsystem &net() const
    {
        assert(network && "ClientArguments::network is null");
        return *network;
    }

    AppState& appState() const
    {
        return appstate;
    }
};
