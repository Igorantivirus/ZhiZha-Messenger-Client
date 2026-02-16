#pragma once

#include "HardStrings.hpp"
#include <Engine/OneRmlDocScene.hpp>

class HelloScene : public engine::OneRmlDocScene
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

        }

    private:
        HelloScene &scene_;
    };

public:
    HelloScene(const engine::Arguments &args) : engine::OneRmlDocScene(args, ui::HelloScene::file), listener_(*this)
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

private:
    HelloSceneListener listener_;
};