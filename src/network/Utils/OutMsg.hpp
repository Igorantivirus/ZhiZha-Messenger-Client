#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace ws
{
struct OutMsg
{
    using Payload = std::variant<std::string, std::vector<std::uint8_t>>;

    Payload payload;

    [[nodiscard]] bool isText() const { return std::holds_alternative<std::string>(payload); }

    [[nodiscard]] std::size_t size() const { return std::visit([](const auto& value) { return value.size(); }, payload); }
};
} // namespace ws
