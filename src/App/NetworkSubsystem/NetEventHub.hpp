#pragma once

#include <utility>
#include <vector>

#include "Core/Types.hpp"
#include "INetEventListener.hpp"

class NetEvent;

// Main-thread event hub. Not thread-safe: subscribe/unsubscribe/dispatch must be called from the same thread.
class NetEventHub
{
public:
    class Subscription
    {
    public:
        Subscription() = default;
        Subscription(NetEventHub &hub, IDType id) : hub_(&hub), id_(id) {}

        Subscription(const Subscription &) = delete;
        Subscription &operator=(const Subscription &) = delete;

        Subscription(Subscription &&other) noexcept { *this = std::move(other); }
        Subscription &operator=(Subscription &&other) noexcept
        {
            if (this == &other)
                return *this;
            reset();
            hub_ = std::exchange(other.hub_, nullptr);
            id_ = std::exchange(other.id_, 0);
            return *this;
        }

        ~Subscription() { reset(); }

        void reset()
        {
            if (!hub_ || id_ == 0)
                return;
            hub_->unsubscribe(id_);
            hub_ = nullptr;
            id_ = 0;
        }

        explicit operator bool() const { return hub_ && id_ != 0; }

    private:
        NetEventHub *hub_ = nullptr;
        IDType id_ = 0;
    };

    Subscription subscribe(INetEventListener &listener)
    {
        const IDType id = nextId_++;
        listeners_.push_back(Entry{id, &listener});
        return Subscription(*this, id);
    }

    void unsubscribe(const IDType id)
    {
        if (id == 0)
            return;
        for (auto it = listeners_.begin(); it != listeners_.end(); ++it)
        {
            if (it->id != id)
                continue;

            if (dispatching_)
            {
                it->id = 0;
                it->ptr = nullptr;
            }
            else
            {
                listeners_.erase(it);
            }
            return;
        }
    }

    void dispatch(const NetEvent &ev)
    {
        dispatching_ = true;
        for (const auto &e : listeners_)
            if (e.ptr)
                e.ptr->onNetEvent(ev);
        dispatching_ = false;

        // Compact any listeners removed during dispatch.
        std::size_t out = 0;
        for (std::size_t i = 0; i < listeners_.size(); ++i)
        {
            if (listeners_[i].ptr)
                listeners_[out++] = listeners_[i];
        }
        listeners_.resize(out);
    }

private:
    struct Entry
    {
        IDType id{};
        INetEventListener *ptr{};
    };

    IDType nextId_ = 1;
    std::vector<Entry> listeners_;
    bool dispatching_ = false;
};

