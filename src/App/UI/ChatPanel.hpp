#pragma once

#include "Core/Types.hpp"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <string>
#include <unordered_map>

struct Message
{
    std::string user;
    std::string msg;
    bool me = false;
};

class ChatPanel
{
public:
    void bind(Rml::ElementDocument &document)
    {
        doc_ = &document;
        messanges_ = document.GetElementById("messages");
        chatList_ = document.GetElementById("chat_list");
        currentChatName_ = document.GetElementById("chat_name");
    }

    void initChatList(const std::map<IDType, std::string> &chats)
    {
        if (!isBound())
            return;

        chatList_->SetInnerRML("");
        chatLabels_.clear();
        activeChatLabel_ = nullptr;

        for (const auto &[id, name] : chats)
            addChat(id, name, false);
    }

    bool isBound() const
    {
        return doc_ && messanges_ && chatList_ && currentChatName_;
    }

    void sendMessage(const IDType chatID, const std::string &userName, const std::string &text, bool isMe)
    {
        if (!isBound())
            return;
        if (chatID == activeChatID_)
            addMsgToCurrentChat(text, userName, isMe);

        Message msg;
        msg.user = userName;
        msg.msg = text;
        msg.me = isMe;
        chatMessages_[chatID].push_back(std::move(msg));
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

    bool updateClickEvent(const std::string &id, const Rml::Element *element)
    {
        auto found = chatLabels_.find(&id);
        if (found != chatLabels_.end())
        {
            makeChatActive(id, found->second);
            return true;
        }
        return false;
    }

    const Rml::Element *getActiveChatLabel() const
    {
        return activeChatLabel_;
    }

    const IDType getActiveChatID() const
    {
        return activeChatID_;
    }

    void chatCreated(const IDType chatID, const std::string &name)
    {
        addChat(chatID, name, false);
    }

private:
    Rml::ElementDocument *doc_ = nullptr;
    Rml::Element *messanges_ = nullptr;
    Rml::Element *chatList_ = nullptr;

    // By string id
    std::unordered_map<const std::string *, Rml::Element *> chatLabels_;
    Rml::Element *activeChatLabel_ = nullptr;
    Rml::Element *currentChatName_ = nullptr;
    IDType activeChatID_ = 0; // aero -- is invalid

    std::unordered_map<IDType, std::vector<Message>> chatMessages_;

private:
    void addMsgToCurrentChat(const std::string text, const std::string &name, const bool isMe)
    {
        // <div class="msg-row">
        Rml::Element *row = messanges_->AppendChild(doc_->CreateElement("div"));

        row->SetClass("msg-row", true);
        row->SetClass(isMe ? "me" : "other", true);

        Rml::Element *msgStack = row->AppendChild(doc_->CreateElement("div"));
        msgStack->SetClass("msg-stack", true);

        //   <div class="msg-name">
        Rml::Element *msgName = msgStack->AppendChild(doc_->CreateElement("div"));
        msgName->SetClass("msg-name", true);
        msgName->AppendChild(doc_->CreateTextNode(name));

        //   <div class="msg">
        Rml::Element *msg = msgStack->AppendChild(doc_->CreateElement("div"));
        msg->SetClass("msg", true);
        msg->AppendChild(doc_->CreateTextNode(text));
    }

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

    void makeChatActive(const std::string &id, Rml::Element *chat)
    {
        if (!isBound())
            return;

        if (activeChatLabel_)
            activeChatLabel_->SetClass("active", false);

        messanges_->SetInnerRML({});

        activeChatLabel_ = chat;
        activeChatLabel_->SetClass("active", true);
        activeChatID_ = std::stoul(id);
        
        auto found = chatMessages_.find(activeChatID_);
        if(found == chatMessages_.end())
            return;
        for(const auto& msg : found->second)
            addMsgToCurrentChat(msg.msg, msg.user, msg.me);
        
        
        
        // messanges_->SetInnerRML(chatMessages_[activeChatID_]);

        // for (std::size_t i = 0; i < activeChatLabel_->GetNumChildren(); ++i)
        // {
        //     auto elem = activeChatLabel_->GetChild(i);
        //     if (elem->IsClassSet("chat-title"))
        //     {
        //         currentChatName_->SetInnerRML(elem->GetInnerRML());
        //         break;
        //     }
        // }
    }

    // void saveCurrent()
    // {
    //     if (!isBound() || activeChatID_ == 0)
    //         return;

    //     std::vector<Message> messages = chatMessages_[activeChatID_];
    //     messages.clear();

    //     for (int i = 0; i < messanges_->GetNumChildren(); ++i)
    //     {
    //         Rml::Element *el = messanges_->GetChild(i);
    //         Message msg;
    //         // msg.user
    //     }
    // }
};