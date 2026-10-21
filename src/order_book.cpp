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

    RemoveResult OrderBook::remove(OrderId id)
    {
        const auto removed_count = orders_.erase(id);

        return removed_count != 0
            ? RemoveResult::Removed
            : RemoveResult::NotFound;
    }

    ReduceResult OrderBook::reduce(OrderId id, Quantity quantity)
    {
        const auto it = orders_.find(id);

        if (it == orders_.end())
        {
            return ReduceResult::NotFound;
        }

        if (quantity == 0 || quantity > it->second.quantity)
        {
            return ReduceResult::InvalidQuantity;
        }

        if (quantity == it->second.quantity)
        {
            orders_.erase(it);
            return ReduceResult::Removed;
        }

        it->second.quantity -= quantity;
        return ReduceResult::Reduced;
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

    std::optional<Price> OrderBook::best_bid() const
    {
        std::optional<Price> best;

        for (const auto& [id, order] : orders_)
        {
            if (order.side == Side::Buy &&
                (!best.has_value() || order.price > *best))
            {
                best = order.price;
            }
        }

        return best;
    }

    std::optional<Price> OrderBook::best_ask() const
    {
        std::optional<Price> best;

        for (const auto& [id, order] : orders_)
        {
            if (order.side == Side::Sell &&
                (!best.has_value() || order.price < *best))
            {
                best = order.price;
            }
        }

        return best;
    }
}