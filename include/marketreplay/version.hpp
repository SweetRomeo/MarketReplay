#pragma once

#include <string_view>

namespace marketreplay
{
    [[nodiscard]] std::string_view version() noexcept;
}