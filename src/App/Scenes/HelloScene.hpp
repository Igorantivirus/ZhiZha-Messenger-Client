#pragma once

#include "AppState.hpp"
#include "HardStrings.hpp"
#include "KeyGenerator.hpp"
#include "NetworkSubsystem/NetEvent.hpp"
#include "NetworkSubsystem/INetEventListener.hpp"
#include <Engine/OneRmlDocScene.hpp>
#include <RmlUi/Core/ElementDocument.h>

#include "ClientArguments.hpp"
#include "network/Utils/WsUrl.hpp"
#include "protocol/JsonMessages.hpp"
#include "protocol/JsonPacker.hpp"
#include "protocol/JsonParser.hpp"

class HelloScene : public engine::OneRmlDocScene, public INetEventListener
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
    HelloScene(const ClientArguments &args) : engine::OneRmlDocScene(args, ui::HelloScene::file), listener_(*this), args_(args), appstate(args.appState())
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
        if (!netSub_)
            netSub_ = args_.netEvents().subscribe(*this);
    }

    void hide() override
    {
        netSub_.reset();
        engine::OneRmlDocScene::hide();
    }

    void onNetEvent(const NetEvent &event) override
    {
        if (event.getType() == NetEvent::Type::onOpen)
        {
            const std::string usernameS = username ? username->GetAttribute("value")->Get<std::string>() : std::string{};
            const std::string passwordS = password ? password->GetAttribute("value")->Get<std::string>() : std::string{};
            (void)passwordS;

            appstate.userName = usernameS;

            ClientRegisterRequest reg;
            reg.clientVersion = "1.0";
            reg.publicKey = "ABOBA";
            reg.username = usernameS;
            args_.net().getClient()->sendText(JsonPacker::packRegisterRequest(reg));
        }
        else if (event.getType() == NetEvent::Type::onText)
        {
            if (const auto *t = event.getIf<NetEvent::OnText>())
                updateServerMsg(t->text);
        }
    }

private:
    HelloSceneListener listener_;
    const ClientArguments &args_;
    AppState &appstate;
    NetEventHub::Subscription netSub_;

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

        args_.net().getClient()->start(std::move(url));
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
