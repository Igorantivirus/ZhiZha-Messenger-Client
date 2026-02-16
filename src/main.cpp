#include "App/Scenes/IDs.hpp"
#include "HardStrings.hpp"
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>

#include <SDLWrapper/SDL3GlobalMeneger.hpp>

#include <Core/Managers/PathMeneger.hpp>
#include <App/ClientEngine.hpp>

#include <App/Scenes/ClientScenesFactory.hpp>
#include <App/HardStrings.hpp>

static ClientEngine application;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (!sdl3::SDL3GlobalMeneger::init(false, false))
    {
        SDL_Log("Error of sdl3::SDL3GlobalMeneger::init");
        return SDL_APP_FAILURE;
    }
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");

    core::managers::PathManager::init();

    auto factory = std::make_unique<ClientScenesFactory>();

    engine::EngineSettings settings;

    settings.appName = names::windowName;
    settings.icoFile = core::managers::PathManager::assets() / names::mainIco;
    settings.fontFile = core::managers::PathManager::assets() / assets::fontPath;
    settings.windowSize = {1000, 800};
    settings.autoOrientationEnabled = false;
    settings.fps = 60;
    settings.startSceneID = scenes::ids::HelloSceneId;
    settings.setLogicalPresentation = false;
    settings.scenesFactory = std::move(factory);

    return application.start(std::move(settings));
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    return application.updateEvents(*event);
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    return application.iterate();
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    application.close();
    sdl3::SDL3GlobalMeneger::shutdown();
}
