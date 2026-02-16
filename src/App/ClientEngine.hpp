#pragma once

#include <Engine/Engine.hpp>

#include <NetworkSubsystem/NetworkSubsystem.hpp>

class ClientEngine : public engine::Engine
{
public:
private:
    NetworkSubsystem subsystem_;

private:
    bool initOptionalSubsystems(const engine::EngineSettings& setts) override
    {
        subsystem_.start();
        return true;
    }

    void shutdownOptionalSubsystems() override
    {
        subsystem_.stop();
    }

    void updateOptionalSubsystems() override
    {
        
    }
};