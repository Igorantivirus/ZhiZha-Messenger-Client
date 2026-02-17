#pragma once

#include "AppState.hpp"
#include "KeyGenerator.hpp"
#include "NetworkSubsystem/NetEvent.hpp"
#include "NetworkSubsystem/NetworkSubsystem.hpp"
#include <Engine/OneRmlDocScene.hpp>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <string>

#include "App/HardStrings.hpp"
#include "ClientArguments.hpp"
#include "network/Utils/WsUrl.hpp"
#include "protocol/JsonMessages.hpp"
#include "protocol/JsonPacker.hpp"
#include "protocol/JsonParser.hpp"

class ChatScene : public engine::OneRmlDocScene
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
    ChatScene(const ClientArguments &args) : engine::OneRmlDocScene(args, ui::ChatScene::file), listener_(*this), network(args.net()), appstate(args.appState())
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
        ThreadSafeQueue<NetEvent> &queue = network.getQueue();
        auto eventOpt = queue.try_pop();
        if (!eventOpt.has_value())
            return engine::OneRmlDocScene::update(dt);

        NetEvent &event = *eventOpt;

        if (event.getType() == NetEvent::Type::onText)
        {
            onMsg(event.get<NetEvent::OnText>()->text);
        }

        // if (event.getType() == NetEvent::Type::onOpen)
        // {
        //     std::string usernameS = username->GetAttribute("value")->Get<std::string>();
        //     std::string passwordS = password->GetAttribute("value")->Get<std::string>();

        //     ClientRegisterRequest reg;
        //     reg.clientVersion = "1.0";
        //     reg.publicKey = "ABOBA";
        //     reg.username = usernameS;
        //     network.getClient()->sendText(JsonPacker::packRegisterRequest(reg));
        //     actionRes_ = engine::SceneAction::nextAction(1);
        // }

        return engine::OneRmlDocScene::update(dt);
    }

private:
    ChatSceneListener listener_;
    NetworkSubsystem &network;
    AppState &appstate;

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
        if (userName == appstate.userName)
            row->SetClass("me", true);
        else
            row->SetClass("other", true);

        //   <div class="msg">
        Rml::Element *msg = row->AppendChild(doc->CreateElement("div"));
        msg->SetClass("msg", true);

        //     Принято.
        msg->AppendChild(doc->CreateTextNode(userName + ": " + text));
    }

    void onMsg(const std::string &s)
    {
        const auto jsonPayload = JsonParser::parseJson(s);
        if (!jsonPayload.has_value())
            return;

        const auto typeopt = JsonParser::parseMessageType(*jsonPayload);
        if (!typeopt.has_value())
            return;
        const std::string &type = typeopt.value();

        if (type == "chat-msg")
        {
            auto request = JsonParser::parseServerChatMessagePayload(*jsonPayload);
            sendToChat(request->userName, request->message);
        }
    }

    void sendMsg()
    {
        if (!messageInput)
            return;
        ClientChatMessageRequest mr;
        mr.chatId = 1;
        mr.userId = appstate.userID;
        mr.message = messageInput->GetAttribute("value")->Get<std::string>();
        if (mr.message.empty())
            return;
        messageInput->SetAttribute("value", "");

        network.getClient()->sendText(JsonPacker::packChatMessageRequest(mr));
    }
};