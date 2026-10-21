#pragma once

#include "marketreplay/order.hpp"

#include <cstddef>
#include <optional>
#include <unordered_map>



namespace marketreplay
{
    enum class RemoveResult
    {
        Removed,
        NotFound
    };

    enum class ReduceResult
    {
        Reduced,
        Removed,
        NotFound,
        InvalidQuantity
    };

    enum class AddResult
    {
        Added,
        InvalidOrder,
        DuplicateId
    };

    class OrderBook
    {
    public:
        [[nodiscard]] AddResult add(const Order& order);

        [[nodiscard]] std::optional<Order> find(OrderId id) const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] RemoveResult remove(OrderId id);

        [[nodiscard]] ReduceResult reduce(OrderId id, Quantity quantity);

    private:
        std::unordered_map<OrderId, Order> orders_;
    };
}