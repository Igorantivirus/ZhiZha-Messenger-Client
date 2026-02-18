#pragma once

class AppEvent;

class IAppEventListener
{
public:
    virtual ~IAppEventListener() = default;
    virtual void onAppEvent(const AppEvent &ev) = 0;
};

