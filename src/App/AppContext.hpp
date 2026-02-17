#pragma once

#include "AppState.hpp"

// App-wide services/state container.
// Kept separate from engine::Arguments to avoid mutable global state inside const arguments.
class AppContext
{
public:
    const AppState &state() const noexcept
    {
        return state_;
    }

    AppState &state() noexcept
    {
        return state_;
    }

private:
    AppState state_{};
};

