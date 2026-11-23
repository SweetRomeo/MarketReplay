#include "marketreplay/order_event.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>

namespace
{
    using namespace marketreplay;
    using Clock = std::chrono::steady_clock;

    constexpr std::size_t order_count = 50'000;
    constexpr Price bid = 1'000'000;
    constexpr Price ask = 1'050'000;
    constexpr Quantity starting_quantity = 50;
    constexpr Quantity cancel_quantity = 15;

    std::vector<OrderEvent> make_events()
    {
        std::vector<OrderEvent> events;
        events.reserve(2 * order_count);
        for (OrderId id = 1; id <= order_count; ++id)
        {
            const bool buy = id % 2 != 0;
            events.emplace_back(AddEvent{Order{id, buy ? Side::Buy : Side::Sell,
                                               buy ? bid : ask, starting_quantity}});
        }
        for (OrderId id = 1; id <= order_count; ++id)
        {
            if (id % 2 != 0)
            {
                events.emplace_back(CancelEvent{id, cancel_quantity});
            }
            else
            {
                events.emplace_back(DeleteEvent{id});
            }
        }
        return events;
    }

    bool verify_replay(const OrderBook& book, const std::vector<EventResult>& results)
    {
        if (results.size() != 2 * order_count || book.size() != order_count / 2 ||
            book.best_bid() != bid || book.best_ask().has_value())
        {
            return false;
        }

        for (std::size_t index = 0; index < order_count; ++index)
        {
            const OrderId id = index + 1;
            if (results[index] != EventResult{AddResult::Added})
            {
                return false;
            }

            const auto& update = results[order_count + index];
            const auto order = book.find(id);
            if (id % 2 != 0)
            {
                if (update != EventResult{ReduceResult::Reduced} || !order ||
                    order->quantity != starting_quantity - cancel_quantity ||
                    order->price != bid || order->side != Side::Buy)
                {
                    return false;
                }
            }
            else if (update != EventResult{RemoveResult::Removed} || order.has_value())
            {
                return false;
            }
        }
        return true;
    }
}

int main()
{
    const auto events = make_events();
    std::array<double, 5> samples_ms{};
    // Run the same workload once before collecting samples.
    for (std::size_t run = 0; run <= samples_ms.size(); ++run)
    {
        marketreplay::OrderBook book;
        const auto start = Clock::now();
        const auto results = marketreplay::replay_events(book, events);
        const auto elapsed = Clock::now() - start;

        if (!verify_replay(book, results))
        {
            std::cerr << "Unexpected event result or final order book in run " << run << '\n';
            return 1;
        }
        if (run > 0)
        {
            samples_ms[run - 1] = std::chrono::duration<double, std::milli>(elapsed).count();
        }
    }

    std::sort(samples_ms.begin(), samples_ms.end());
    const double median_ms = samples_ms[samples_ms.size() / 2];
    std::cout << "Events per replay: " << events.size()
              << "\nRemaining orders: " << order_count / 2
              << "\nMeasured runs: " << samples_ms.size()
              << "\nMedian replay time (ms): " << std::fixed << std::setprecision(3) << median_ms
              << "\nEvents per second: " << std::setprecision(0)
              << (1000.0 * events.size() / median_ms) << '\n';
}
