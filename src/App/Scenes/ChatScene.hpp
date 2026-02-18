#pragma once

#include <App/Events/AppEvent.hpp>
#include <App/Events/AppEventHub.hpp>
#include <App/Events/IAppEventListener.hpp>
#include <Engine/OneRmlDocScene.hpp>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <string>

#include "App/HardStrings.hpp"
#include "ClientArguments.hpp"

class ChatScene : public engine::OneRmlDocScene, public IAppEventListener
{
private:
    class ChatSceneListener : public Rml::EventListener
    {
    public:
        ChatSceneListener(ChatScene &scene) : scene_(scene)
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
            if (id == "send_button")
            {
                scene_.sendMsg();
            }
        }

    private:
        ChatScene &scene_;
    };

public:
    ChatScene(const ClientArguments &args) : engine::OneRmlDocScene(args, ui::ChatScene::file), listener_(*this), args_(args)
    {
        loadDocumentOrThrow();
        addEventListener(Rml::EventId::Click, &listener_, true);
    }

    void updateEvent(const SDL_Event &event) override
    {
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            // Проверка нажатия Enter (возврат каретки)
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER)
                sendMsg();
        }
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
        if (event.getType() != AppEvent::Type::ChatMessageReceived)
            return;
        const auto *m = event.getIf<AppEvent::ChatMessageReceived>();
        if (!m)
            return;
        sendToChat(m->userName, m->message);
    }

private:
    ChatSceneListener listener_;
    const ClientArguments &args_;
    AppEventHub::Subscription appSub_;

    Rml::Element *messageInput = nullptr;
    Rml::Element *messanges = nullptr;

private:
    void onDocumentLoaded(Rml::ElementDocument &doc) override
    {
        messageInput = doc.GetElementById("message_input");
        messanges = doc.GetElementById("messages");
    }

    void sendToChat(const std::string &userName, const std::string &text)
    {
        if (!messanges)
            return;
        Rml::ElementDocument *doc = document();
        if (!doc)
            return;

        // <div class="msg-row">
        Rml::Element *row = messanges->AppendChild(doc->CreateElement("div"));
        row->SetClass("msg-row", true);
        if (userName == args_.appState().userName)
            row->SetClass("me", true);
        else
            row->SetClass("other", true);

        Rml::Element* msgStack = row->AppendChild(doc->CreateElement("div"));
        msgStack->SetClass("msg-stack", true);

        //   <div class="msg-name">
        Rml::Element *msgName = msgStack->AppendChild(doc->CreateElement("div"));
        msgName->SetClass("msg-name", true);
        msgName->AppendChild(doc->CreateTextNode(userName));

        //   <div class="msg">
        Rml::Element *msg = msgStack->AppendChild(doc->CreateElement("div"));
        msg->SetClass("msg", true);
        msg->AppendChild(doc->CreateTextNode(text));
    }

    void sendMsg()
    {
        if (!messageInput)
            return;
        const std::string msg = messageInput->GetAttribute("value")->Get<std::string>();
        if (msg.empty())
            return;
        messageInput->SetAttribute("value", "");
        args_.appContext().chat().sendMessage(msg);
    }
};
