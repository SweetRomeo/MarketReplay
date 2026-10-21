//
// Created by berke on 9.10.2026.
//
#include "marketreplay/order_book.hpp"

#include <gtest/gtest.h>
#include <initializer_list>

namespace marketreplay
{
    namespace
    {
        void expect_order(const Order& actual, const Order& expected)
        {
            EXPECT_EQ(actual.id, expected.id);
            EXPECT_EQ(actual.side, expected.side);
            EXPECT_EQ(actual.price, expected.price);
            EXPECT_EQ(actual.quantity, expected.quantity);
        }
    }

    TEST(OrderBookTest, StartsEmpty)
    {
        const OrderBook book;

        EXPECT_TRUE(book.empty());
        EXPECT_EQ(book.size(), 0U);
        EXPECT_FALSE(book.find(1).has_value());
    }

    TEST(OrderBookTest, AddsAndFindsBuyOrder)
    {
        OrderBook book;
        const Order order{1, Side::Buy, 1'234'567, 100};

        ASSERT_EQ(book.add(order), AddResult::Added);

        const auto found = book.find(order.id);
        ASSERT_TRUE(found.has_value());
        expect_order(*found, order);

        EXPECT_FALSE(book.empty());
        EXPECT_EQ(book.size(), 1U);
    }

    TEST(OrderBookTest, AddsAndFindsSellOrder)
    {
        OrderBook book;
        const Order order{2, Side::Sell, 1'234'568, 50};

        ASSERT_EQ(book.add(order), AddResult::Added);

        const auto found = book.find(order.id);
        ASSERT_TRUE(found.has_value());
        expect_order(*found, order);
    }

    TEST(OrderBookTest, StoresDistinctIds)
    {
        OrderBook book;
        const Order buy{1, Side::Buy, 1'234'567, 100};
        const Order sell{2, Side::Sell, 1'234'568, 50};

        ASSERT_EQ(book.add(buy), AddResult::Added);
        ASSERT_EQ(book.add(sell), AddResult::Added);
        EXPECT_EQ(book.size(), 2U);

        const auto found_buy = book.find(buy.id);
        const auto found_sell = book.find(sell.id);

        ASSERT_TRUE(found_buy.has_value());
        ASSERT_TRUE(found_sell.has_value());
        expect_order(*found_buy, buy);
        expect_order(*found_sell, sell);
    }

    TEST(OrderBookTest, RejectsIdenticalDuplicate)
    {
        OrderBook book;
        const Order order{1, Side::Buy, 1'234'567, 100};

        ASSERT_EQ(book.add(order), AddResult::Added);
        EXPECT_EQ(book.add(order), AddResult::DuplicateId);
        EXPECT_EQ(book.size(), 1U);

        const auto found = book.find(order.id);
        ASSERT_TRUE(found.has_value());
        expect_order(*found, order);
    }

    TEST(OrderBookTest, DuplicateDoesNotReplaceOriginal)
    {
        OrderBook book;
        const Order original{1, Side::Buy, 1'234'567, 100};
        const Order duplicate{1, Side::Sell, 2'000'000, 50};

        ASSERT_EQ(book.add(original), AddResult::Added);
        EXPECT_EQ(book.add(duplicate), AddResult::DuplicateId);
        EXPECT_EQ(book.size(), 1U);

        const auto found = book.find(original.id);
        ASSERT_TRUE(found.has_value());
        expect_order(*found, original);
    }

    TEST(OrderBookTest, RejectsInvalidOrdersWithoutChangingState)
    {
        OrderBook book;
        const Order original{10, Side::Buy, 1'234'567, 100};

        ASSERT_EQ(book.add(original), AddResult::Added);

        const Order invalid_orders[]{
            {0, Side::Buy, 1'234'567, 100},
            {2, Side::Buy, 0, 100},
            {3, Side::Buy, -1, 100},
            {4, Side::Buy, 1'234'567, 0},
            {5, static_cast<Side>(255), 1'234'567, 100}
        };

        for (const auto& order : invalid_orders)
        {
            SCOPED_TRACE(order.id);

            EXPECT_EQ(book.add(order), AddResult::InvalidOrder);
            EXPECT_EQ(book.size(), 1U);
            EXPECT_FALSE(book.find(order.id).has_value());
        }

        const auto found = book.find(original.id);
        ASSERT_TRUE(found.has_value());
        expect_order(*found, original);
    }

    TEST(OrderBookTest, InvalidOrderTakesPrecedenceOverDuplicateId)
    {
        OrderBook book;
        const Order original{1, Side::Buy, 1'234'567, 100};
        const Order invalid_duplicate{1, Side::Buy, 0, 100};

        ASSERT_EQ(book.add(original), AddResult::Added);

        EXPECT_EQ(
            book.add(invalid_duplicate),
            AddResult::InvalidOrder
        );
        EXPECT_EQ(book.size(), 1U);

        const auto found = book.find(original.id);
        ASSERT_TRUE(found.has_value());
        expect_order(*found, original);
    }

    TEST(OrderBookTest, MissingLookupDoesNotChangeState)
    {
        OrderBook book;
        const Order order{1, Side::Buy, 1'234'567, 100};

        ASSERT_EQ(book.add(order), AddResult::Added);

        EXPECT_FALSE(book.find(999).has_value());
        EXPECT_EQ(book.size(), 1U);

        const auto found = book.find(order.id);
        ASSERT_TRUE(found.has_value());
        expect_order(*found, order);
    }

    TEST(OrderBookTest, ReturnedCopyDoesNotChangeStoredOrder)
    {
        OrderBook book;
        const Order original{1, Side::Buy, 1'234'567, 100};

        ASSERT_EQ(book.add(original), AddResult::Added);

        auto copy = book.find(original.id);
        ASSERT_TRUE(copy.has_value());

        copy->id = 99;
        copy->side = Side::Sell;
        copy->price = 1;
        copy->quantity = 1;

        const auto stored = book.find(original.id);
        ASSERT_TRUE(stored.has_value());
        expect_order(*stored, original);

        EXPECT_FALSE(book.find(99).has_value());
        EXPECT_EQ(book.size(), 1U);
    }

    TEST(OrderBookTest, RemovesBuyAndSellOrders)
    {
        for (const auto side : {Side::Buy, Side::Sell})
        {
            OrderBook book;
            const Order order{1, side, 1'234'567, 100};

            ASSERT_EQ(book.add(order), AddResult::Added);

            EXPECT_EQ(book.remove(order.id), RemoveResult::Removed);
            EXPECT_FALSE(book.find(order.id).has_value());
            EXPECT_EQ(book.size(), 0U);
            EXPECT_TRUE(book.empty());

            EXPECT_EQ(book.remove(order.id), RemoveResult::NotFound);
            EXPECT_EQ(book.reduce(order.id, 1), ReduceResult::NotFound);
        }
    }

    TEST(OrderBookTest, RemovingMissingOrderPreservesStoredOrder)
    {
        OrderBook book;
        const Order original{1, Side::Buy, 1'234'567, 100};

        ASSERT_EQ(book.add(original), AddResult::Added);

        EXPECT_EQ(book.remove(999), RemoveResult::NotFound);
        EXPECT_EQ(book.size(), 1U);

        const auto found = book.find(original.id);
        ASSERT_TRUE(found.has_value());
        expect_order(*found, original);
    }

    TEST(OrderBookTest, PartialReductionChangesOnlyQuantity)
    {
        for (const auto side : {Side::Buy, Side::Sell})
        {
            OrderBook book;
            const Order original{1, side, 1'234'567, 100};

            ASSERT_EQ(book.add(original), AddResult::Added);
            EXPECT_EQ(book.reduce(original.id, 30), ReduceResult::Reduced);

            auto expected = original;
            expected.quantity = 70;

            const auto found = book.find(original.id);
            ASSERT_TRUE(found.has_value());
            expect_order(*found, expected);
            EXPECT_EQ(book.size(), 1U);
        }
    }

    TEST(OrderBookTest, ExactReductionRemovesOrder)
    {
        OrderBook book;
        const Order order{1, Side::Sell, 1'234'567, 100};

        ASSERT_EQ(book.add(order), AddResult::Added);

        EXPECT_EQ(book.reduce(order.id, 100), ReduceResult::Removed);
        EXPECT_FALSE(book.find(order.id).has_value());
        EXPECT_EQ(book.size(), 0U);
        EXPECT_TRUE(book.empty());

        EXPECT_EQ(book.reduce(order.id, 1), ReduceResult::NotFound);
        EXPECT_EQ(book.remove(order.id), RemoveResult::NotFound);
    }

    TEST(OrderBookTest, InvalidReductionsPreserveOrder)
    {
        OrderBook book;
        const Order original{1, Side::Buy, 1'234'567, 100};

        ASSERT_EQ(book.add(original), AddResult::Added);

        const Quantity invalid_quantities[]{0, 101};

        for (const auto quantity : invalid_quantities)
        {
            SCOPED_TRACE(quantity);

            EXPECT_EQ(
                book.reduce(original.id, quantity),
                ReduceResult::InvalidQuantity
            );
            EXPECT_EQ(book.size(), 1U);

            const auto found = book.find(original.id);
            ASSERT_TRUE(found.has_value());
            expect_order(*found, original);
        }
    }

    TEST(OrderBookTest, MissingIdTakesPrecedenceOverInvalidQuantity)
    {
        OrderBook book;
        const Order original{1, Side::Buy, 1'234'567, 100};

        ASSERT_EQ(book.add(original), AddResult::Added);

        EXPECT_EQ(book.reduce(999, 0), ReduceResult::NotFound);
        EXPECT_EQ(book.reduce(999, 10), ReduceResult::NotFound);
        EXPECT_EQ(book.size(), 1U);

        const auto found = book.find(original.id);
        ASSERT_TRUE(found.has_value());
        expect_order(*found, original);
    }

    TEST(OrderBookTest, RepeatedReductionsEventuallyRemoveOrder)
    {
        OrderBook book;
        const Order original{1, Side::Buy, 1'234'567, 100};

        ASSERT_EQ(book.add(original), AddResult::Added);

        EXPECT_EQ(book.reduce(original.id, 30), ReduceResult::Reduced);

        auto expected = original;
        expected.quantity = 70;

        const auto after_first = book.find(original.id);
        ASSERT_TRUE(after_first.has_value());
        expect_order(*after_first, expected);
        EXPECT_EQ(book.size(), 1U);

        EXPECT_EQ(book.reduce(original.id, 20), ReduceResult::Reduced);

        expected.quantity = 50;

        const auto after_second = book.find(original.id);
        ASSERT_TRUE(after_second.has_value());
        expect_order(*after_second, expected);
        EXPECT_EQ(book.size(), 1U);

        EXPECT_EQ(book.reduce(original.id, 50), ReduceResult::Removed);
        EXPECT_TRUE(book.empty());
        EXPECT_EQ(book.reduce(original.id, 1), ReduceResult::NotFound);
    }

    TEST(OrderBookTest, OperationsPreserveUnrelatedOrders)
    {
        OrderBook book;
        const Order target{1, Side::Buy, 1'234'567, 100};
        const Order unrelated{2, Side::Sell, 1'234'568, 50};

        ASSERT_EQ(book.add(target), AddResult::Added);
        ASSERT_EQ(book.add(unrelated), AddResult::Added);

        EXPECT_EQ(book.reduce(target.id, 30), ReduceResult::Reduced);
        EXPECT_EQ(book.size(), 2U);

        const auto after_reduce = book.find(unrelated.id);
        ASSERT_TRUE(after_reduce.has_value());
        expect_order(*after_reduce, unrelated);

        EXPECT_EQ(book.remove(target.id), RemoveResult::Removed);
        EXPECT_EQ(book.size(), 1U);

        const auto after_remove = book.find(unrelated.id);
        ASSERT_TRUE(after_remove.has_value());
        expect_order(*after_remove, unrelated);

        ASSERT_EQ(book.add(target), AddResult::Added);
        EXPECT_EQ(book.reduce(target.id, 100), ReduceResult::Removed);
        EXPECT_EQ(book.size(), 1U);

        const auto after_exact_reduce = book.find(unrelated.id);
        ASSERT_TRUE(after_exact_reduce.has_value());
        expect_order(*after_exact_reduce, unrelated);
    }

    TEST(OrderBookTest, DeletedIdCanBeAddedAgain)
    {
        OrderBook book;
        const Order original{1, Side::Buy, 1'234'567, 100};
        const Order replacement{1, Side::Sell, 2'000'000, 50};

        ASSERT_EQ(book.add(original), AddResult::Added);
        ASSERT_EQ(book.remove(original.id), RemoveResult::Removed);
        ASSERT_EQ(book.add(replacement), AddResult::Added);

        const auto found = book.find(replacement.id);
        ASSERT_TRUE(found.has_value());
        expect_order(*found, replacement);
        EXPECT_EQ(book.size(), 1U);

        ASSERT_EQ(book.reduce(replacement.id, 50), ReduceResult::Removed);
        EXPECT_EQ(book.add(original), AddResult::Added);
    }
}