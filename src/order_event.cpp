//
// Created by berke on 22.10.2026.
//
#include "marketreplay/order_event.hpp"

#include <type_traits>

namespace marketreplay
{
    EventResult apply_event(OrderBook& book, const OrderEvent& event)
    {
        return std::visit(
            [&book](const auto& value) -> EventResult
            {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, AddEvent>)
                {
                    return book.add(value.order);
                }
                else if constexpr (std::is_same_v<T, CancelEvent>)
                {
                    return book.reduce(value.id, value.quantity);
                }
                else
                {
                    static_assert(std::is_same_v<T, DeleteEvent>);
                    return book.remove(value.id);
                }
            },
            event
        );
    }

    std::vector<EventResult> replay_events(
    OrderBook& book,
    std::span<const OrderEvent> events)
    {
        std::vector<EventResult> results;
        results.reserve(events.size());

        for (const auto& event : events)
        {
            results.push_back(apply_event(book, event));
        }

        return results;
    }
}
