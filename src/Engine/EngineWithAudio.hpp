#pragma once

#include <SDLWrapper/Audio/AudioDevice.hpp>

#include "Engine.hpp"

namespace engine
{

// Optional helper engine that wires an SDLWrapper audio device into the base Engine.
// If you need more control, derive from engine::Engine directly and implement the same hooks.
class EngineWithAudio : public Engine
{
public:
    sdl3::audio::AudioDevice &audio()
    {
        return audio_;
    }

protected:
    bool initOptionalSubsystems(const EngineSettings & /*setts*/) override
    {
        return audio_.initTracks(4);
    }

    void shutdownOptionalSubsystems() override
    {
        audio_.close();
    }

    void updateOptionalSubsystems() override
    {
        audio_.update();
    }

    sdl3::audio::AudioDevice *audioDevice() override
    {
        return &audio_;
    }

private:
    sdl3::audio::AudioDevice audio_;
};

} // namespace engine

