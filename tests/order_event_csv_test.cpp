#include "marketreplay/order_event_csv.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <variant>

namespace marketreplay {

    TEST(OrderEventCsvTest, HeaderOnlyReturnsEmptyEvents)
    {
        std::istringstream input(
            "type,id,side,price,quantity\n"
        );

        const auto result = parse_order_events_csv(input);

        ASSERT_TRUE(
            std::holds_alternative<std::vector<OrderEvent>>(result)
        );

        EXPECT_TRUE(
            std::get<std::vector<OrderEvent>>(result).empty()
        );
    }

    TEST(OrderEventCsvTest, InvalidHeaderReturnsError)
    {
        std::istringstream input("wrong,header\n");

        const auto result = parse_order_events_csv(input);

        ASSERT_TRUE(
            std::holds_alternative<CsvParseError>(result)
        );

        const auto& error = std::get<CsvParseError>(result);

        EXPECT_EQ(error.line, 1U);
        EXPECT_EQ(error.reason, "Invalid CSV header");
    }

    TEST(OrderEventCsvTest, RejectsIncorrectFieldCount)
    {
        std::istringstream input(
            "type,id,side,price,quantity\n"
            "DELETE,2,,\n"
        );

        const auto result = parse_order_events_csv(input);

        ASSERT_TRUE(std::holds_alternative<CsvParseError>(result));

        const auto& error = std::get<CsvParseError>(result);

        EXPECT_EQ(error.line, 2U);
        EXPECT_EQ(error.reason, "Expected exactly 5 fields");
    }

}