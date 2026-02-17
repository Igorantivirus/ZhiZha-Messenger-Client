#pragma once

#include <App/NetworkSubsystem/NetEvent.hpp>
#include <App/NetworkSubsystem/ThreadSafeQueue.hpp>
#include <App/ClientArguments.hpp>
#include <App/AppContext.hpp>
#include <Engine/Engine.hpp>

class ClientEngine : public engine::Engine
{
public:
private:
    ClientArguments args_;
    AppContext app_;
    NetworkSubsystem subsystem_;
    NetEventHub netHub_;

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
        clientArgs->netHub = &netHub_;
        clientArgs->app = &app_;
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
        // Incoming: net thread -> queue -> main thread hub.
        ThreadSafeQueue<NetEvent> &queue = subsystem_.getQueue();
        while(!queue.empty())
        {
            auto evOpt = queue.try_pop();
            if (!evOpt.has_value())
                break;
            netHub_.dispatch(*evOpt);
        }
    }
};
