#pragma once

#include <memory>

#include "Arguments.hpp"
#include "Core/Types.hpp"
#include "Scene.hpp"

namespace engine
{

class SceneFactory
{
public:
    virtual ~SceneFactory() = default;

    // Create a scene by ID. The engine passes the same `Arguments` instance to all scenes.
    virtual ScenePtr createSceneByID(const IDType id, const Arguments &args) const = 0;
};

using SceneFactoryPtr = std::unique_ptr<SceneFactory>;

} // namespace engine

