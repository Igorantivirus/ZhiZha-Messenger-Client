#pragma once

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

class ChatPanel
{
public:
    void bind(Rml::ElementDocument &document)
    {
        doc_ = &document;
        messanges_ = document.GetElementById("messages");
    }

    bool isBound() const
    {
        return doc_ && messanges_;
    }

    void sendMessage(const std::string &userName, const std::string &text, bool isMe)
    {
        if(!isBound())
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
        if(messanges_)
            messanges_->SetInnerRML("");
    }

    std::string getAllMessangesToSave()
    {
        return messanges_ ? messanges_->GetInnerRML() : std::string{};
    }

private:

    Rml::ElementDocument *doc_ = nullptr;
    Rml::Element *messanges_ = nullptr;
};