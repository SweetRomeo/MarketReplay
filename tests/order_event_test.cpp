//
// Created by berke on 22.10.2026.
//
#include "marketreplay/order_event.hpp"

#include <gtest/gtest.h>

#include <variant>

namespace marketreplay {
    TEST(OrderEventTest, AddEventStoresOrder) {
        OrderBook book;
        const Order order{1, Side::Buy, 100, 50};

        const auto result = apply_event(book, AddEvent{order});

        ASSERT_TRUE(std::holds_alternative<AddResult>(result));
        EXPECT_EQ(std::get<AddResult>(result), AddResult::Added);

        const auto stored = book.find(order.id);
        ASSERT_TRUE(stored.has_value());
        EXPECT_EQ(stored->id, order.id);
        EXPECT_EQ(stored->side, order.side);
        EXPECT_EQ(stored->price, order.price);
        EXPECT_EQ(stored->quantity, order.quantity);
        EXPECT_EQ(book.size(), 1U);
    }

    TEST(OrderEventTest, CancelEventSubtractsQuantity) {
        OrderBook book;
        ASSERT_EQ(book.add({1, Side::Buy, 100, 50}), AddResult::Added);

        const auto result = apply_event(book, CancelEvent{1, 20});

        ASSERT_TRUE(std::holds_alternative<ReduceResult>(result));
        EXPECT_EQ(std::get<ReduceResult>(result), ReduceResult::Reduced);

        const auto stored = book.find(1);
        ASSERT_TRUE(stored.has_value());
        EXPECT_EQ(stored->quantity, 30U);
        EXPECT_EQ(book.size(), 1U);
    }

    TEST(OrderEventTest, ExactCancelEventRemovesOrder) {
        OrderBook book;
        ASSERT_EQ(book.add({1, Side::Sell, 105, 50}), AddResult::Added);

        const auto result = apply_event(book, CancelEvent{1, 50});

        ASSERT_TRUE(std::holds_alternative<ReduceResult>(result));
        EXPECT_EQ(std::get<ReduceResult>(result), ReduceResult::Removed);
        EXPECT_TRUE(book.empty());
    }

    TEST(OrderEventTest, DeleteEventRemovesOrder) {
        OrderBook book;
        ASSERT_EQ(book.add({1, Side::Sell, 105, 50}), AddResult::Added);

        const auto result = apply_event(book, DeleteEvent{1});

        ASSERT_TRUE(std::holds_alternative<RemoveResult>(result));
        EXPECT_EQ(std::get<RemoveResult>(result), RemoveResult::Removed);
        EXPECT_FALSE(book.find(1).has_value());
        EXPECT_TRUE(book.empty());
    }

    TEST(OrderEventTest, RejectedEventsPreserveStoredOrder) {
        OrderBook book;
        const Order original{1, Side::Buy, 100, 50};
        ASSERT_EQ(book.add(original), AddResult::Added);

        EXPECT_EQ(
            apply_event(book, AddEvent{Order{1, Side::Sell, 105, 20}}),
            EventResult{AddResult::DuplicateId}
        );
        EXPECT_EQ(
            apply_event(book, AddEvent{Order{2, Side::Buy, 100, 0}}),
            EventResult{AddResult::InvalidOrder}
        );
        EXPECT_EQ(
            apply_event(book, CancelEvent{1, 0}),
            EventResult{ReduceResult::InvalidQuantity}
        );
        EXPECT_EQ(
            apply_event(book, CancelEvent{1, 51}),
            EventResult{ReduceResult::InvalidQuantity}
        );
        EXPECT_EQ(
            apply_event(book, CancelEvent{999, 0}),
            EventResult{ReduceResult::NotFound}
        );
        EXPECT_EQ(
            apply_event(book, DeleteEvent{999}),
            EventResult{RemoveResult::NotFound}
        );

        const auto stored = book.find(original.id);
        ASSERT_TRUE(stored.has_value());
        EXPECT_EQ(stored->id, original.id);
        EXPECT_EQ(stored->side, original.side);
        EXPECT_EQ(stored->price, original.price);
        EXPECT_EQ(stored->quantity, original.quantity);
        EXPECT_EQ(book.size(), 1U);
    }

    TEST(OrderEventTest, EmptyReplayPreservesExistingBook) {
        OrderBook book;
        const Order original{1, Side::Buy, 100, 50};
        ASSERT_EQ(book.add(original), AddResult::Added);

        const auto results = replay_events(
            book,
            std::span<const OrderEvent>{}
        );

        EXPECT_TRUE(results.empty());
        EXPECT_EQ(book.size(), 1U);

        const auto stored = book.find(original.id);
        ASSERT_TRUE(stored.has_value());
        EXPECT_EQ(stored->id, original.id);
        EXPECT_EQ(stored->side, original.side);
        EXPECT_EQ(stored->price, original.price);
        EXPECT_EQ(stored->quantity, original.quantity);
    }

    TEST(OrderEventTest, ReplayAppliesEventsInInputOrder) {
        OrderBook book;
        const OrderEvent events[]{
            AddEvent{Order{1, Side::Buy, 100, 50}},
            AddEvent{Order{2, Side::Buy, 102, 30}},
            CancelEvent{2, 10},
            DeleteEvent{1}
        };

        const auto results = replay_events(book, events);

        const std::vector<EventResult> expected{
            AddResult::Added,
            AddResult::Added,
            ReduceResult::Reduced,
            RemoveResult::Removed
        };

        EXPECT_EQ(results, expected);
        EXPECT_EQ(book.size(), 1U);
        EXPECT_FALSE(book.find(1).has_value());

        const auto stored = book.find(2);
        ASSERT_TRUE(stored.has_value());
        EXPECT_EQ(stored->id, 2U);
        EXPECT_EQ(stored->side, Side::Buy);
        EXPECT_EQ(stored->price, 102);
        EXPECT_EQ(stored->quantity, 20U);
        EXPECT_EQ(book.best_bid(), std::optional<Price>{102});
        EXPECT_FALSE(book.best_ask().has_value());
    }

    TEST(OrderEventTest, ReplayContinuesAfterRejectedEvents) {
        OrderBook book;
        const OrderEvent events[]{
            DeleteEvent{99},
            AddEvent{Order{1, Side::Buy, 100, 50}},
            AddEvent{Order{1, Side::Sell, 105, 20}},
            CancelEvent{1, 51},
            CancelEvent{1, 20},
            CancelEvent{1, 30},
            DeleteEvent{1},
            AddEvent{Order{1, Side::Sell, 105, 10}}
        };

        const auto results = replay_events(book, events);

        const std::vector<EventResult> expected{
            RemoveResult::NotFound,
            AddResult::Added,
            AddResult::DuplicateId,
            ReduceResult::InvalidQuantity,
            ReduceResult::Reduced,
            ReduceResult::Removed,
            RemoveResult::NotFound,
            AddResult::Added
        };

        EXPECT_EQ(results, expected);
        EXPECT_EQ(book.size(), 1U);

        const auto stored = book.find(1);
        ASSERT_TRUE(stored.has_value());
        EXPECT_EQ(stored->side, Side::Sell);
        EXPECT_EQ(stored->price, 105);
        EXPECT_EQ(stored->quantity, 10U);
        EXPECT_FALSE(book.best_bid().has_value());
        EXPECT_EQ(book.best_ask(), std::optional<Price>{105});
    }

    TEST(OrderEventTest, ReplayContinuesFromExistingBook) {
        OrderBook book;
        ASSERT_EQ(book.add({1, Side::Buy, 100, 50}), AddResult::Added);
        ASSERT_EQ(book.add({2, Side::Sell, 105, 20}), AddResult::Added);

        const std::vector<OrderEvent> events{
            CancelEvent{1, 10},
            AddEvent{Order{3, Side::Buy, 102, 30}}
        };

        const auto results = replay_events(book, events);

        const std::vector<EventResult> expected{
            ReduceResult::Reduced,
            AddResult::Added
        };

        EXPECT_EQ(results, expected);
        EXPECT_EQ(book.size(), 3U);

        const auto buy = book.find(1);
        ASSERT_TRUE(buy.has_value());
        EXPECT_EQ(buy->quantity, 40U);

        const auto sell = book.find(2);
        ASSERT_TRUE(sell.has_value());
        EXPECT_EQ(sell->id, 2U);
        EXPECT_EQ(sell->side, Side::Sell);
        EXPECT_EQ(sell->price, 105);
        EXPECT_EQ(sell->quantity, 20U);

        EXPECT_EQ(book.best_bid(), std::optional<Price>{102});
        EXPECT_EQ(book.best_ask(), std::optional<Price>{105});
    }

    TEST(OrderEventTest, SameEventsAndInitialStateProduceSameResults) {
        OrderBook first;
        OrderBook second;

        const Order initial{10, Side::Sell, 110, 25};
        ASSERT_EQ(first.add(initial), AddResult::Added);
        ASSERT_EQ(second.add(initial), AddResult::Added);

        const OrderEvent events[]{
            AddEvent{Order{1, Side::Buy, 100, 50}},
            CancelEvent{1, 20},
            DeleteEvent{10},
            AddEvent{Order{2, Side::Sell, 105, 15}},
            CancelEvent{99, 0}
        };

        const auto first_results = replay_events(first, events);
        const auto second_results = replay_events(second, events);

        EXPECT_EQ(first_results, second_results);
        EXPECT_EQ(first.size(), 2U);
        EXPECT_EQ(second.size(), 2U);

        for (const OrderId id: {OrderId{1}, OrderId{2}, OrderId{10}}) {
            const auto a = first.find(id);
            const auto b = second.find(id);

            ASSERT_EQ(a.has_value(), b.has_value());

            if (a.has_value()) {
                EXPECT_EQ(a->id, b->id);
                EXPECT_EQ(a->side, b->side);
                EXPECT_EQ(a->price, b->price);
                EXPECT_EQ(a->quantity, b->quantity);
            }
        }

        EXPECT_EQ(first.best_bid(), second.best_bid());
        EXPECT_EQ(first.best_ask(), second.best_ask());
    }
}
