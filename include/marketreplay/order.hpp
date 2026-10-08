#pragma once

#include <cstdint>

namespace marketreplay
{
    using OrderId = std::uint64_t;
    using Price = std::int64_t;
    using Quantity = std::uint32_t;

    inline constexpr Price price_scale = 10'000;

    enum class Side : std::uint8_t
    {
        Buy,
        Sell
    };

    // Represents an order for a single instrument.
    // Price is stored in units of 1 / price_scale.
    struct Order
    {
        OrderId id{};
        Side side{Side::Buy};
        Price price{};
        Quantity quantity{};
    };

    [[nodiscard]] constexpr bool is_valid(const Order& order) noexcept
    {
        const bool valid_side =
            order.side == Side::Buy || order.side == Side::Sell;

        return order.id != 0
            && valid_side
            && order.price > 0
            && order.quantity > 0;
    }
}