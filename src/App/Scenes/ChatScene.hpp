#pragma once

#include <App/Events/AppEvent.hpp>
#include <App/Events/AppEventHub.hpp>
#include <App/Events/IAppEventListener.hpp>
#include <Engine/OneRmlDocScene.hpp>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <boost/beast/websocket/rfc6455.hpp>
#include <string>

#include "App/HardStrings.hpp"
#include "ClientArguments.hpp"
#include "Core/Types.hpp"

#include <UI/ChatPanel.hpp>
#include <UI/CreateGroupOverlay.hpp>

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
            if(scene_.chatPanel.updateClickEvent(id, el))
                return;
            if(scene_.createGroupOverlay_.updateClickEvent(id, el))
                return;
            if (id == "send_button")
            {
                scene_.sendMsg();
            }
            else if(id == "server-leave")
            {
                scene_.leaveFromServer();
            }
            else if(id == "open_group_panel")
            {
                scene_.startCreateRoom();
            }
        }

    private:
        ChatScene &scene_;
    };

public:
    ChatScene(const ClientArguments &args) : engine::OneRmlDocScene(args, ui::ChatScene::file), listener_(*this), createGroupOverlay_(args), args_(args)
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
        if (event.getType() == AppEvent::Type::ChatMessageReceived)
        {
            const auto *m = event.getIf<AppEvent::ChatMessageReceived>();
            if (!m)
                return;
            chatPanel.sendMessage(m->chatID, m->userName, m->message, m->userName == args_.appState().userName);
        }
        else if (event.getType() == AppEvent::Type::ChatsPayload)
        {
            const auto *m = event.getIf<AppEvent::ChatsPayload>();
            if (!m)
                return;
            chatPanel.initChatList(args_.appState().chats);
        }
        else if (event.getType() == AppEvent::Type::RoomCreated)
        {
            const auto *c = event.getIf<AppEvent::RoomCreated>();
            if (!c)
                return;
            chatPanel.chatCreated(c->chatID, c->name);


            // chatPanel.initChatList(args_.appState().chats);
        }
    }

private:
    ChatSceneListener listener_;
    const ClientArguments &args_;
    AppEventHub::Subscription appSub_;

    Rml::Element *messageInput = nullptr;

    ChatPanel chatPanel;
    CreateGroupOverlay createGroupOverlay_;

    
private:
    void onDocumentLoaded(Rml::ElementDocument &doc) override
    {
        messageInput = doc.GetElementById("message_input");
        chatPanel.bind(doc);
        createGroupOverlay_.bind(doc);
        args_.appContext().chat().sendChatsRequest();
        args_.appContext().chat().sendUsersRequest();
    }

    void leaveFromServer()
    {
        args_.net().getClient()->stop(ws::websocket::close_code::normal, "user leave from server");
        actionRes_ = engine::SceneAction::popAction();
    }

    void sendMsg()
    {
        if (!messageInput || chatPanel.getActiveChatID() == 0)
            return;
        const std::string msg = messageInput->GetAttribute("value")->Get<std::string>();
        if (msg.empty())
            return;
        messageInput->SetAttribute("value", "");
        const IDType id = chatPanel.getActiveChatID();
        args_.appContext().chat().sendMessage(msg, id);
    }

    void startCreateRoom()
    {
        createGroupOverlay_.open(args_.appContext().state().users);
    }
};
