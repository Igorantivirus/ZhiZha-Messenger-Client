#pragma once

#include "Core/Types.hpp"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <string>
#include <unordered_map>

class ChatPanel
{
public:
    void bind(Rml::ElementDocument &document)
    {
        doc_ = &document;
        messanges_ = document.GetElementById("messages");
        chatList_ = document.GetElementById("chat_list");
    }

    void initChatList(const std::map<IDType, std::string> &chats)
    {
        if (!isBound())
            return;

        chatList_->SetInnerRML("");
        chatLabels_.clear();
        activeElement = nullptr;

        for (const auto &[id, name] : chats)
            addChat(id, name, false);
    }

    bool isBound() const
    {
        return doc_ && messanges_ && chatList_;
    }

    void sendMessage(const IDType chatID, const std::string &userName, const std::string &text, bool isMe)
    {
        if (!isBound())
            return;
        if(chatID != activeChatID_)
            return;

        // <div class="msg-row">
        Rml::Element *row = messanges_->AppendChild(doc_->CreateElement("div"));

        row->SetClass("msg-row", true);
        row->SetClass(isMe ? "me" : "other", true);

        Rml::Element *msgStack = row->AppendChild(doc_->CreateElement("div"));
        msgStack->SetClass("msg-stack", true);

        //   <div class="msg-name">
        Rml::Element *msgName = msgStack->AppendChild(doc_->CreateElement("div"));
        msgName->SetClass("msg-name", true);
        msgName->AppendChild(doc_->CreateTextNode(userName));

        //   <div class="msg">
        Rml::Element *msg = msgStack->AppendChild(doc_->CreateElement("div"));
        msg->SetClass("msg", true);
        msg->AppendChild(doc_->CreateTextNode(text));
    }

    void clearMessanges()
    {
        if (messanges_)
            messanges_->SetInnerRML("");
    }

    std::string getAllMessangesToSave()
    {
        return messanges_ ? messanges_->GetInnerRML() : std::string{};
    }

    bool updateClickEvent(const std::string& id, const Rml::Element* element)
    {
        auto found = chatLabels_.find(&id);
        if(found != chatLabels_.end())
        {
            makeChatActibe(id, found->second);
            return true;
        }
        return false;
    }

    const Rml::Element *getActiveElement() const
    {
        return activeElement;
    }

    const IDType getActiveChatID() const
    {
        return activeChatID_;
    }

private:
    Rml::ElementDocument *doc_ = nullptr;
    Rml::Element *messanges_ = nullptr;
    Rml::Element *chatList_ = nullptr;

    // By string id
    std::unordered_map<const std::string *, Rml::Element *> chatLabels_;
    Rml::Element *activeElement = nullptr;
    IDType activeChatID_ = 0;//aero -- is invalid

private:
    void addChat(const IDType id, const std::string &name, bool active)
    {
        /*
        <div class="chat-item active">
            <div class="chat-title">Общий</div>
            <div class="chat-last">Последнее: «Ок, понял»</div>
        </div>
        */

        Rml::Element *chatItem = chatList_->AppendChild(doc_->CreateElement("div"));
        chatItem->SetClass("chat-item", true);
        chatItem->SetClass("active", active);
        chatItem->SetId(std::to_string(id));

        Rml::Element *chatTitle = chatItem->AppendChild(doc_->CreateElement("div"));
        chatTitle->SetClass("chat-title", true);
        chatTitle->SetInnerRML(name);

        Rml::Element *chatLast = chatItem->AppendChild(doc_->CreateElement("div"));
        chatTitle->SetClass("chat-last", true);

        chatLabels_[&chatItem->GetId()] = chatItem;
    }

    void makeChatActibe(const std::string& id, Rml::Element* chat)
    {
        if(activeElement)
            activeElement->SetClass("active", false);
        activeElement = chat;
        activeElement->SetClass("active", true);
        activeChatID_ = std::stoul(id);
    }
};