#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "IAppEventListener.hpp"

class AppEvent;

// Main-thread app event hub. Not thread-safe: subscribe/unsubscribe/dispatch must be called from the same thread.
class AppEventHub
{
public:
    class Subscription
    {
    public:
        Subscription() = default;
        Subscription(AppEventHub &hub, std::uint64_t id) : hub_(&hub), id_(id) {}

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
        AppEventHub *hub_ = nullptr;
        std::uint64_t id_ = 0;
    };

    Subscription subscribe(IAppEventListener &listener)
    {
        const std::uint64_t id = nextId_++;
        listeners_.push_back(Entry{id, &listener});
        return Subscription(*this, id);
    }

    void unsubscribe(const std::uint64_t id)
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

    void dispatch(const AppEvent &ev)
    {
        dispatching_ = true;
        for (const auto &e : listeners_)
        {
            if (e.ptr)
                e.ptr->onAppEvent(ev);
        }
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
        std::uint64_t id{};
        IAppEventListener *ptr{};
    };

    std::uint64_t nextId_ = 1;
    std::vector<Entry> listeners_;
    bool dispatching_ = false;
};

