#pragma once

class NetEvent;

// Main-thread interface: implement in scenes (or controllers) to receive NetEvent.
class INetEventListener
{
public:
    virtual ~INetEventListener() = default;
    virtual void onNetEvent(const NetEvent &ev) = 0;
};

