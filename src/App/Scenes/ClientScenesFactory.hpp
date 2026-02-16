#pragma once

#include <Scenes/IDs.hpp>
#include <Scenes/HelloScene.hpp>
#include <Engine/EmptyScene.hpp>
#include <Engine/SceneFactory.hpp>

class ClientScenesFactory : public engine::SceneFactory
{
public:

    engine::ScenePtr createSceneByID(const IDType id, const engine::Arguments &args) const override
    {
        if(id == scenes::ids::HelloSceneId)
            return std::make_unique<HelloScene>(args);
        return std::make_unique<engine::EmptyScene>(args);
    }

};
