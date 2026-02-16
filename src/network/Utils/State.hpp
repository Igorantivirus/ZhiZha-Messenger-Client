#pragma once

namespace ws
{
enum class State
{
    idle,
    connecting,
    open,
    resolving,
    handshaking,
    closing,
    closed,
    error
};

} // namespace ws
