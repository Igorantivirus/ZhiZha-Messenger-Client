#pragma once

#include <memory>

#include <RmlUi/Core/ElementDocument.h>
#include <SDLWrapper/SDLWrapper.hpp>

#include "Arguments.hpp"
#include "SceneAction.hpp"

namespace engine
{

class Scene
{
public:
    explicit Scene(const Arguments &args)
        : args_(&args)
    {
    }
    Scene(const Scene &) = delete;
    Scene &operator=(const Scene &) = delete;
    Scene(Scene &&) = delete;
    Scene &operator=(Scene &&) = delete;

    virtual ~Scene() = default;

    virtual void updateEvent(const SDL_Event &event) = 0;
    virtual SceneAction update(const float dt)
    {
        return actionRes_;
    }

    virtual void draw(sdl3::RenderWindow &window) const = 0;

    virtual void hide()
    {
    }
    virtual void show()
    {
    }

protected:
    const Arguments &args() const
    {
        return *args_;
    }

    SceneAction actionRes_ = SceneAction::noneAction();

private:
    const Arguments *args_ = nullptr;
};

using ScenePtr = std::unique_ptr<Scene>;

} // namespace engine
