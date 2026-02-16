#pragma once

#include "Scene.hpp"

namespace engine
{

class EmptyScene final : public Scene
{
public:
    explicit EmptyScene(const Arguments &args)
        : Scene(args)
    {
    }

    void updateEvent(const SDL_Event & /*event*/) override
    {
    }

    void draw(sdl3::RenderWindow & /*window*/) const override
    {
    }
};

} // namespace engine

