#pragma once

#include <App/ClientArguments.hpp>
#include <Engine/Engine.hpp>

class ClientEngine : public engine::Engine
{
public:
private:
    ClientArguments args_;
    NetworkSubsystem subsystem_;

private:
    engine::Arguments &arguments() override
    {
        return args_;
    }

    const engine::Arguments &arguments() const override
    {
        return args_;
    }

    void configureArguments(engine::Arguments &args, const engine::EngineSettings & /*setts*/) override
    {
        auto *clientArgs = args.as<ClientArguments>();
        if (!clientArgs)
            return;
        clientArgs->network = &subsystem_;
    }

    bool initOptionalSubsystems(const engine::EngineSettings &setts) override
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
