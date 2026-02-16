#pragma once

#include <cassert>

#include <SDLWrapper/Renders/RenderWindow.hpp>

namespace sdl3::audio
{
class AudioDevice;
}

namespace engine
{

class Context;

// A single "bag of dependencies" passed to every Scene and SceneFactory.
// Users may derive from this type to add their own fields, while all scenes can
// still accept `const engine::Arguments&`.
struct Arguments
{
    virtual ~Arguments() = default;

    Context *context = nullptr;
    sdl3::RenderWindow *window = nullptr;
    sdl3::audio::AudioDevice *audio = nullptr; // optional

    Context &ctx() const
    {
        assert(context && "engine::Arguments::context is null");
        return *context;
    }

    sdl3::RenderWindow &wnd() const
    {
        assert(window && "engine::Arguments::window is null");
        return *window;
    }

    template <class T>
    const T *as() const
    {
        return dynamic_cast<const T *>(this);
    }
};

} // namespace engine

