#pragma once

#include "ClientArguments.hpp"
#include "Core/Types.hpp"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <map>
#include <string>


class CreateGroupOverlay
{
public:
    CreateGroupOverlay(const ClientArguments& args) : args_(args)
    {}

    void bind(Rml::ElementDocument &doc)
    {
        doc_ = &doc;
        overlay_ = doc.GetElementById("create-group-overlay");
        createB_  = doc.GetElementById("create-group-btn");
        backB_ = doc.GetElementById("cancel-group-btn");
        userList_ = doc.GetElementById("user-list");
        roomNameInput_ = doc.GetElementById("room-name");
        isPrivateCheck_ = dynamic_cast<Rml::ElementFormControlInput*>(doc.GetElementById("private-room"));
    }

    bool isBound() const
    {
        return doc_ && overlay_ && createB_ && backB_ && roomNameInput_ && isPrivateCheck_;
    }

    void open(const std::map<IDType, std::string> &users)
    {
        if (!isBound())
            return;
        overlay_->SetClass("hidden", false);
        userLabels_.clear();
        userList_->SetInnerRML("");
        for(const auto& [id, name] : users)
            createUser(name, id);

    }

    void close()
    {
        if (!overlay_)
            return;
        overlay_->SetClass("hidden", true);
    }

    // Call from a scene's click handler; returns true if the overlay handled the click.
    bool updateClickEvent(const Rml::String &id, Rml::Element *el)
    {
        if (!isBound())
            return false;
        if(el == backB_)
        {
            close();
            return true;
        }
        if(el == createB_)
        {
            bool res = tryCreate();
            if(res)
                close();
            return res;
        }
        auto found = userLabels_.find(&id);
        if(found == userLabels_.end())
            return false;
        changeSelected(el);
        return true;
    }

private:
    Rml::ElementDocument *doc_ = nullptr;

    Rml::Element *overlay_ = nullptr;
    Rml::Element *createB_ = nullptr;
    Rml::Element *backB_ = nullptr;
    Rml::Element *userList_ = nullptr;
    Rml::Element *roomNameInput_ = nullptr;
    Rml::ElementFormControlInput* isPrivateCheck_ = nullptr;

    std::unordered_map<const std::string *, Rml::Element *> userLabels_;

    const ClientArguments& args_;

private:

    bool tryCreate()
    {
        std::string roomName = roomNameInput_->GetAttribute("value")->Get<std::string>();
        if(roomName.empty())
            return false;

        bool isPrivate = isPrivateCheck_->GetValue() == "on";
        std::vector<IDType> users;
        for(const auto& [idP, el] : userLabels_)
            if(el->IsClassSet("selected"))
                users.push_back(el->GetAttribute("user-id")->Get<IDType>());

        if(users.empty())
            return false;

        args_.appContext().chat().sendCreateRoomRequest(isPrivate, std::move(users), std::move(roomName));

        return true;
    }

    void changeSelected(Rml::Element *el)
    {
        bool selected = el->IsClassSet("selected");
        el->SetClass("selected", !selected);
    }

    void createUser(const std::string& name, const IDType id)
    {    
        if(!isBound())
            return;
        //<div class="user-item">Анна</div>
        Rml::Element *userItem = userList_->AppendChild(doc_->CreateElement("div"));
        userItem->SetClass("user-item", true);
        userItem->SetClass("selected", false);
        userItem->SetId("user-to-add-" + std::to_string(id));
        userItem->SetAttribute("user-id", id);
        userItem->SetInnerRML(name);

        userLabels_[&userItem->GetId()] = userItem;
    }


};