//
// Created by berke on 22.10.2026.
//

#pragma once

#include "marketreplay/order.hpp"
#include "marketreplay/order_book.hpp"

#include <variant>
#include <span>
#include <vector>

namespace marketreplay
{
    struct AddEvent
    {
        Order order;
    };

    struct CancelEvent
    {
        OrderId id;
        Quantity quantity;
    };

    struct DeleteEvent
    {
        OrderId id;
    };

    using OrderEvent = std::variant<AddEvent, CancelEvent, DeleteEvent>;
    using EventResult = std::variant<AddResult, ReduceResult, RemoveResult>;

    [[nodiscard]] EventResult apply_event(
        OrderBook& book,
        const OrderEvent& event
    );

    [[nodiscard]] std::vector<EventResult> replay_events(
        OrderBook& book,
        std::span<const OrderEvent> events
    );
}
