#pragma once

#include "AppState.hpp"
#include "HardStrings.hpp"
#include "KeyGenerator.hpp"
#include "NetworkSubsystem/NetEvent.hpp"
#include "NetworkSubsystem/NetworkSubsystem.hpp"
#include <Engine/OneRmlDocScene.hpp>
#include <RmlUi/Core/ElementDocument.h>

#include "ClientArguments.hpp"
#include "network/Utils/WsUrl.hpp"
#include "protocol/JsonMessages.hpp"
#include "protocol/JsonPacker.hpp"
#include "protocol/JsonParser.hpp"

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
            if (id == "connect-btn")
            {
                scene_.tryConnect();
            }
        }

    private:
        HelloScene &scene_;
    };

public:
    HelloScene(const ClientArguments &args) : engine::OneRmlDocScene(args, ui::HelloScene::file), listener_(*this), network(args.net()), appstate(args.appState())
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
        ThreadSafeQueue<NetEvent> &queue = network.getQueue();
        auto eventOpt = queue.try_pop();
        if (!eventOpt.has_value())
            return engine::OneRmlDocScene::update(dt);

        NetEvent &event = *eventOpt;

        if (event.getType() == NetEvent::Type::onOpen)
        {
            std::string usernameS = username->GetAttribute("value")->Get<std::string>();
            std::string passwordS = password->GetAttribute("value")->Get<std::string>();

            appstate.userName = usernameS;

            ClientRegisterRequest reg;
            reg.clientVersion = "1.0";
            reg.publicKey = "ABOBA";
            reg.username = usernameS;
            network.getClient()->sendText(JsonPacker::packRegisterRequest(reg));
            //
        }
        if (event.getType() == NetEvent::Type::onText)
            updateServerMsg(event.get<NetEvent::OnText>()->text);

        return engine::OneRmlDocScene::update(dt);
    }

private:
    HelloSceneListener listener_;
    NetworkSubsystem &network;
    AppState& appstate;

    Rml::Element *username = nullptr;
    Rml::Element *server = nullptr;
    Rml::Element *password = nullptr;

private:
    void onDocumentLoaded(Rml::ElementDocument &doc) override
    {
        username = doc.GetElementById("username");
        server = doc.GetElementById("server");
        password = doc.GetElementById("password");
    }

    void tryConnect()
    {
        std::string serverS = server->GetAttribute("value")->Get<std::string>();
        ws::WsUrl url = KeyGenerator::fromKey(serverS);

        // ClientRegisterRequest reg;
        // reg.clientVersion = "1.0";
        // reg.publicKey = "ABOBA";
        // reg.username = usernameS;

        network.getClient()->start(std::move(url));
    }

    void updateServerMsg(const std::string& s)
    {
        const auto jsonPayload = JsonParser::parseJson(s);
        if (!jsonPayload.has_value())
            return;

        const auto typeopt = JsonParser::parseMessageType(*jsonPayload);
        if (!typeopt.has_value())
            return;
        const std::string &type = typeopt.value();

        if (type == "register-result")
        {
            auto request = JsonParser::parseServerRegistrationPayload(*jsonPayload);
            
            appstate.userID = request->userId;
            actionRes_ = engine::SceneAction::nextAction(1);
        }
    }
};