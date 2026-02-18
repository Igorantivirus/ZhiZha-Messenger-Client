#pragma once

#include <string>

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

// Reusable controller for `assets/ui/overlays/message-overlay.rml`.
// Assumes standard element ids: message-overlay, message-tittle, message-text, notice_ok_btn.
class MessageOverlay
{
public:
    void bind(Rml::ElementDocument &doc)
    {
        overlay_ = doc.GetElementById("message-overlay");
        title_ = doc.GetElementById("message-tittle");
        text_ = doc.GetElementById("message-text");
        okBtn_ = doc.GetElementById("notice_ok_btn");
    }

    bool isBound() const
    {
        return overlay_ && title_ && text_ && okBtn_;
    }

    void open(const std::string& title, const std::string& msg)
    {
        if (!isBound())
            return;
        overlay_->SetClass("hidden", false);
        title_->SetInnerRML(title);
        text_->SetInnerRML(msg);
    }

    void close()
    {
        if (!overlay_)
            return;
        overlay_->SetClass("hidden", true);
    }

    // Call from a scene's click handler; returns true if the overlay handled the click.
    bool handleClickId(const Rml::String &id)
    {
        if (!okBtn_)
            return false;
        if (id == okBtn_->GetId())
        {
            close();
            return true;
        }
        return false;
    }

private:
    Rml::Element *overlay_ = nullptr;
    Rml::Element *title_ = nullptr;
    Rml::Element *text_ = nullptr;
    Rml::Element *okBtn_ = nullptr;
};

