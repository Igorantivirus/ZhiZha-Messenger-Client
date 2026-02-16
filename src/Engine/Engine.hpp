#pragma once

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDLWrapper/Math/Colors.hpp>
#include <SDLWrapper/Names.hpp>
#include <SDLWrapper/Renders/VideoMode.hpp>
#include <SDLWrapper/Renders/View.hpp>
#include <string_view>
#include <vector>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>

#include <SDLWrapper/Clock.hpp>
#include <SDLWrapper/SDLWrapper.hpp>

#include "AdvancedContext.hpp"
#include "Core/Types.hpp"
#include "Engine/EngineSettings.hpp"
#include "EngineSettings.hpp"
#include "Arguments.hpp"
#include "Scene.hpp"
#include "SceneAction.hpp"
#include "SceneFactory.hpp"
#include "WindowSizeInfo.hpp"

namespace engine
{

class Engine
{
public:
    void close()
    {
        scenes_.clear();
        shutdownOptionalSubsystems();
        context_.quit();
        window_.close();
    }

    SDL_AppResult start(EngineSettings setts)
    {
        winSizeInfo_.init(setts.windowSize);

        sdl3::VideoMode mode = sdl3::VideoMode::getDefaultVideoMode();
        mode.width = setts.windowSize.x;
        mode.height = setts.windowSize.y;

        if (!window_.create(std::move(setts.appName), mode))
            return SDL_APP_FAILURE;
        if (!initOptionalSubsystems(setts))
            return SDL_APP_FAILURE;
        if (!context_.init(window_.getNativeSDLWindow(), window_.getNativeSDLRenderer(), setts.fontFile))
            return SDL_APP_FAILURE;
        if (!window_.loadIconFromFile(setts.icoFile.string()))
            SDL_Log("Error of open icon");
        if (setts.setLogicalPresentation)
        {
            window_.setLogicalPresentation(setts.windowSize, setts.mode);
            auto view = window_.getView();
            auto logicalSize = window_.getLogicSize();
            view.setCenterPosition({logicalSize.x / 2.f, logicalSize.y / 2.f});
            window_.setView(view);
        }
        mode_ = setts.mode;
        autoOrientationEnabled_ = setts.autoOrientationEnabled;

        registerSceneFactory(std::move(setts.scenesFactory));
        setFps(setts.fps);
        bindArguments_(setts);
        if (pushScene(setts.startSceneID) != SDL_APP_CONTINUE)
            return SDL_APP_FAILURE;

        return SDL_APP_CONTINUE;
    }

    // SET METHODS

    void registerSceneFactory(SceneFactoryPtr ptr)
    {
        sceneFactory_ = std::move(ptr);
    }
    void setFps(const unsigned int fps)
    {
        fps_ = fps;
        if (fps_ > 0)
            desiredFrameMS_ = 1000.f / static_cast<float>(fps_);
    }
    void setAutoOrientationEnabled(const bool enabled)
    {
        autoOrientationEnabled_ = enabled;
    }
    bool isAutoOrientationEnabled() const
    {
        return autoOrientationEnabled_;
    }

    // ITERATE METHODS

    SDL_AppResult updateEvents(SDL_Event &event)
    {
        if (event.type == SDL_EVENT_QUIT)
            return SDL_APP_SUCCESS;
        if (autoOrientationEnabled_ && event.type == SDL_EVENT_WINDOW_RESIZED)
            handleWindowResize(event.window.data1, event.window.data2);
        if (scenes_.empty())
            return SDL_APP_FAILURE;
        window_.convertEventToRenderCoordinates(&event);
        context_.updateEvents(event);
        window_.convertEventToViewCoordinates(&event);
        scenes_.back()->updateEvent(event);
        return SDL_APP_CONTINUE;
    }

    SDL_AppResult iterate()
    {
        if (scenes_.empty())
            return SDL_APP_FAILURE;
        const float dt = cl_.elapsedTimeS();
        cl_.start();
        SceneAction act = scenes_.back()->update(dt);
        SDL_AppResult res = processSceneAction(act);
        if (res != SDL_APP_CONTINUE)
            return res;
        context_.update();
        updateOptionalSubsystems();
        safeDrawScene();
        fpsDelay();
        return res;
    }

    // GET METHODS

    sdl3::RenderWindow &getWindow()
    {
        return window_;
    }

    const unsigned int getFps() const
    {
        return fps_;
    }

protected:
    // Override in derived engine if you want a custom Arguments type.
    virtual Arguments &arguments()
    {
        return args_;
    }

    virtual const Arguments &arguments() const
    {
        return args_;
    }

    // Optional subsystems (audio, networking, etc.). Base Engine does not force any of them.
    virtual bool initOptionalSubsystems(const EngineSettings & /*setts*/)
    {
        return true;
    }

    virtual void shutdownOptionalSubsystems()
    {
    }

    virtual void updateOptionalSubsystems()
    {
    }

    // If you have an audio device, override and return a pointer for scene access via Arguments.
    virtual sdl3::audio::AudioDevice *audioDevice()
    {
        return nullptr;
    }

    // Let derived engines populate their own extra fields in Arguments.
    virtual void configureArguments(Arguments & /*args*/, const EngineSettings & /*setts*/)
    {
    }

private:
    std::vector<ScenePtr> scenes_;
    SceneFactoryPtr sceneFactory_;

    // желаемое время кадра (мс)
    float desiredFrameMS_{};
    unsigned int fps_{};

private:
    sdl3::RenderWindow window_;
    Context context_;
    sdl3::ClockNS cl_;
    Arguments args_;

    WindowSizeInfo winSizeInfo_;
    SDL_RendererLogicalPresentation mode_;
    bool autoOrientationEnabled_ = true;

private:
    void handleWindowResize(const int windowW, const int windowH)
    {
        if (!winSizeInfo_.handleWindowResize(windowH, windowW))
            return;

        auto view = window_.getView();
        view.setCenterPosition(winSizeInfo_.centerPos);
        window_.setLogicalPresentation(winSizeInfo_.windowLogicslSize, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        window_.setView(view);
    }

    void safeDrawScene()
    {
        window_.clear(sdl3::Colors::White);
        scenes_.back()->draw(window_);
        context_.render();
        window_.display();
    }

    void fpsDelay()
    {
        if (fps_ == 0)
            return;

        // фактическое время, затраченное на кадр (мс)
        float frameMS = cl_.elapsedTimeMS();

        if (frameMS < desiredFrameMS_)
            SDL_Delay(static_cast<Uint32>(desiredFrameMS_ - frameMS));
    }

private:
    void bindArguments_(const EngineSettings &setts)
    {
        Arguments &a = arguments();
        a.context = &context_;
        a.window = &window_;
        a.audio = audioDevice();
        configureArguments(a, setts);
    }

private: // SceneActionType process
    SDL_AppResult processSceneAction(const SceneAction &act)
    {
        if (act.type == SceneActionType::None)
            return SDL_APP_CONTINUE;
        if (act.type == SceneActionType::PushScene)
            return pushScene(std::get<IDType>(act.value));
        if (act.type == SceneActionType::PopScene)
            return popScene();
        if (act.type == SceneActionType::SwitchScene)
            return switchScene(std::get<IDType>(act.value));
        if (act.type == SceneActionType::Exit)
            return SDL_APP_SUCCESS;
        return SDL_APP_CONTINUE;
    }

    SDL_AppResult pushScene(const IDType sceneId)
    {
        if (!sceneFactory_)
        {
            SDL_Log("SceneFactory is null");
            return SDL_APP_FAILURE;
        }

        if (!scenes_.empty())
            scenes_.back()->hide();

        ScenePtr next = sceneFactory_->createSceneByID(sceneId, arguments());
        if (!next)
        {
            SDL_Log("SceneFactory returned null scene for id=%d", static_cast<int>(sceneId));
            return SDL_APP_FAILURE;
        }

        scenes_.push_back(std::move(next));
        scenes_.back()->show();
        return SDL_APP_CONTINUE;
    }

    SDL_AppResult popScene()
    {
        if (scenes_.empty())
            return SDL_APP_FAILURE;
        scenes_.back()->hide();
        scenes_.pop_back();
        if (!scenes_.empty())
            scenes_.back()->show();
        return scenes_.empty() ? SDL_APP_FAILURE : SDL_APP_CONTINUE;
    }

    SDL_AppResult switchScene(const IDType sceneId)
    {
        if (scenes_.empty())
            return SDL_APP_FAILURE;
        scenes_.back()->hide();
        scenes_.pop_back();
        return pushScene(sceneId);
    }
};

} // namespace engine
