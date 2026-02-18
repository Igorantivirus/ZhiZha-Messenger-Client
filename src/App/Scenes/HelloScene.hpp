#pragma once

#include "HardStrings.hpp"
#include <App/Events/AppEvent.hpp>
#include <App/Events/AppEventHub.hpp>
#include <App/Events/IAppEventListener.hpp>
#include <App/UI/MessageOverlay.hpp>
#include <Engine/OneRmlDocScene.hpp>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

#include "ClientArguments.hpp"

class HelloScene : public engine::OneRmlDocScene, public IAppEventListener
{
private:
    class HelloSceneListener : public Rml::EventListener
    {
    public:
        HelloSceneListener(HelloScene &scene) : scene_(scene)
        {
        }
        void ProcessEvent(Rml::Event &ev) override
        {
            Rml::Element *el = ev.GetTargetElement();
            while (el && el->GetId().empty())
                el = el->GetParentNode();
            if (!el)
                return;
            const Rml::String &id = el->GetId();

            if (scene_.messageOverlay_.handleClickId(id))
                return;

            if (id == "connect-btn")
                scene_.tryConnect();
        }

    private:
        HelloScene &scene_;
    };

public:
    HelloScene(const ClientArguments &args) : engine::OneRmlDocScene(args, ui::HelloScene::file), listener_(*this), args_(args)
    {
        loadDocumentOrThrow();
        addEventListener(Rml::EventId::Click, &listener_, true);
    }

    void updateEvent(const SDL_Event &event) override
    {
    }

    void draw(sdl3::RenderWindow &window) const override
    {
    }

    engine::SceneAction update(const float dt) override
    {
        return engine::OneRmlDocScene::update(dt);
    }

    void show() override
    {
        engine::OneRmlDocScene::show();
        if (!appSub_)
            appSub_ = args_.appContext().events().subscribe(*this);
    }

    void hide() override
    {
        appSub_.reset();
        engine::OneRmlDocScene::hide();
    }

    void onAppEvent(const AppEvent &event) override
    {
        if (event.getType() == AppEvent::Type::RegisterSucceeded)
        {
            const auto *ok = event.getIf<AppEvent::RegisterSucceeded>();
            if (!ok)
                return;
            (void)ok;
            actionRes_ = engine::SceneAction::nextAction(1);
        }
        else if (event.getType() == AppEvent::Type::RegisterFailed)
        {
            const auto *error = event.getIf<AppEvent::RegisterFailed>();
            if (!error)
                return;
            messageOverlay_.open(error->code, error->message);
        }
    }

private:
    void onDocumentLoaded(Rml::ElementDocument &doc) override
    {
        username = doc.GetElementById("username");
        server = doc.GetElementById("server");
        password = doc.GetElementById("password");
        messageOverlay_.bind(doc);
    }

    void tryConnect()
    {
        const std::string usernameS = username ? username->GetAttribute("value")->Get<std::string>() : std::string{};
        const std::string serverS = server ? server->GetAttribute("value")->Get<std::string>() : std::string{};
        const std::string passwordS = password ? password->GetAttribute("value")->Get<std::string>() : std::string{};
        (void)passwordS;

        args_.appContext().registration().connectAndRegister(serverS, usernameS, passwordS);
    }

private:
    HelloSceneListener listener_;
    const ClientArguments &args_;
    AppEventHub::Subscription appSub_;

    Rml::Element *username = nullptr;
    Rml::Element *server = nullptr;
    Rml::Element *password = nullptr;

    MessageOverlay messageOverlay_;
};
