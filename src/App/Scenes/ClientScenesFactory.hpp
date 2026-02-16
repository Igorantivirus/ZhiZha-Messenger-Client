#pragma once

#include <Engine/EmptyScene.hpp>
#include <Engine/SceneFactory.hpp>
#include <Scenes/ChatScene.hpp>
#include <Scenes/HelloScene.hpp>
#include <Scenes/IDs.hpp>

class ClientScenesFactory : public engine::SceneFactory
{
public:
    engine::ScenePtr createSceneByID(const IDType id, const engine::Arguments &args) const override
    {
        const ClientArguments* cargsPtr = args.as<ClientArguments>(); 
        if(!cargsPtr)
            return std::make_unique<engine::EmptyScene>(args);
        const ClientArguments& cargs = *cargsPtr;

        if (id == scenes::ids::HelloSceneId)
            return std::make_unique<HelloScene>(cargs);
        if (id == scenes::ids::ChatSceneId)
            return std::make_unique<ChatScene>(cargs);
        return std::make_unique<engine::EmptyScene>(args);
    }
};
