//
// Created by berke on 9.10.2026.
//
#include "marketreplay/order_book.hpp"

namespace marketreplay
{
    AddResult OrderBook::add(const Order& order)
    {
        if (!is_valid(order))
        {
            return AddResult::InvalidOrder;
        }

        const auto result = orders_.try_emplace(order.id, order);

        return result.second
            ? AddResult::Added
            : AddResult::DuplicateId;
    }

    std::optional<Order> OrderBook::find(OrderId id) const
    {
        const auto it = orders_.find(id);

        if (it == orders_.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    std::size_t OrderBook::size() const noexcept
    {
        return orders_.size();
    }

    bool OrderBook::empty() const noexcept
    {
        return orders_.empty();
    }
}