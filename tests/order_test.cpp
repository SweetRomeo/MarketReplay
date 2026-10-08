//
// Created by berke on 8.10.2026.
//
#include "marketreplay/order.hpp"

#include <gtest/gtest.h>

namespace marketreplay
{
    TEST(OrderTest, AcceptsValidBuyOrder)
    {
        const Order order{1, Side::Buy, 1'234'567, 100};

        EXPECT_TRUE(is_valid(order));
    }

    TEST(OrderTest, AcceptsValidSellOrder)
    {
        const Order order{2, Side::Sell, 1'234'568, 50};

        EXPECT_TRUE(is_valid(order));
    }

    TEST(OrderTest, AcceptsMinimumPositiveValues)
    {
        const Order order{1, Side::Buy, 1, 1};

        EXPECT_TRUE(is_valid(order));
    }

    TEST(OrderTest, RejectsDefaultOrder)
    {
        EXPECT_FALSE(is_valid(Order{}));
    }

    TEST(OrderTest, RejectsZeroId)
    {
        const Order order{0, Side::Buy, 1'234'567, 100};

        EXPECT_FALSE(is_valid(order));
    }

    TEST(OrderTest, RejectsZeroPrice)
    {
        const Order order{1, Side::Buy, 0, 100};

        EXPECT_FALSE(is_valid(order));
    }

    TEST(OrderTest, RejectsNegativePrice)
    {
        const Order order{1, Side::Buy, -1, 100};

        EXPECT_FALSE(is_valid(order));
    }

    TEST(OrderTest, RejectsZeroQuantity)
    {
        const Order order{1, Side::Buy, 1'234'567, 0};

        EXPECT_FALSE(is_valid(order));
    }

    TEST(OrderTest, RejectsInvalidSide)
    {
        const Order order{
            1,
            static_cast<Side>(255),
            1'234'567,
            100
        };

        EXPECT_FALSE(is_valid(order));
    }
}