#pragma once

#include "Core/Types.hpp"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <map>
#include <string>


class CreateGroupOverlay
{
public:
    void bind(Rml::ElementDocument &doc)
    {
        doc_ = &doc;
        overlay_ = doc.GetElementById("create-group-overlay");
        createB_  = doc.GetElementById("create-group-btn");
        backB_ = doc.GetElementById("cancel-group-btn");
        userList_ = doc.GetElementById("user-list");
    }

    bool isBound() const
    {
        return doc_ && overlay_ && createB_ && backB_;
    }

    void open(const std::map<IDType, std::string> &users)
    {
        if (!isBound())
            return;
        overlay_->SetClass("hidden", false);
        // title_->SetInnerRML(title);
        // text_->SetInnerRML(msg);
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
            close();
            return true;
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

    std::unordered_map<const std::string *, Rml::Element *> userLabels_;

private:

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
        userItem->SetInnerRML(name);

        userLabels_[&userItem->GetId()] = userItem;
    }


};