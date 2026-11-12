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

    TEST(OrderEventCsvTest, ParsesMixedEventsInOrder)
    {
        std::istringstream input(
            "type,id,side,price,quantity\n"
            "ADD,1,BUY,1000000,50\n"
            "CANCEL,1,,,10\n"
            "DELETE,1,,,\n"
        );

        const auto result = parse_order_events_csv(input);

        ASSERT_TRUE(
            std::holds_alternative<std::vector<OrderEvent>>(result)
        );

        const auto& events = std::get<std::vector<OrderEvent>>(result);
        ASSERT_EQ(events.size(), 3U);

        ASSERT_TRUE(std::holds_alternative<AddEvent>(events[0]));
        EXPECT_EQ(std::get<AddEvent>(events[0]).order.id, 1U);

        ASSERT_TRUE(std::holds_alternative<CancelEvent>(events[1]));
        EXPECT_EQ(std::get<CancelEvent>(events[1]).id, 1U);
        EXPECT_EQ(std::get<CancelEvent>(events[1]).quantity, 10U);

        ASSERT_TRUE(std::holds_alternative<DeleteEvent>(events[2]));
        EXPECT_EQ(std::get<DeleteEvent>(events[2]).id, 1U);
    }

    TEST(OrderEventCsvTest, RejectsInvalidCancelDeleteAndUnknownTypes)
    {
        struct Case {
            const char* row;
            const char* reason;
        };

        const Case cases[] = {
            {"CANCEL,0,,,10", "Invalid CANCEL id"},
            {"CANCEL,1,BUY,,10", "CANCEL side and price must be empty"},
            {"CANCEL,1,,100,10", "CANCEL side and price must be empty"},
            {"CANCEL,1,,,0", "Invalid CANCEL quantity"},
            {"CANCEL,1,,,4294967296", "Invalid CANCEL quantity"},
            {"DELETE,-1,,,", "Invalid DELETE id"},
            {"DELETE,1,BUY,,", "DELETE side, price and quantity must be empty"},
            {"DELETE,1,,100,", "DELETE side, price and quantity must be empty"},
            {"DELETE,1,,,10", "DELETE side, price and quantity must be empty"},
            {"UPDATE,1,,,", "Unknown event type"},
        };

        for (const auto& testCase : cases) {
            SCOPED_TRACE(testCase.row);

            std::istringstream input(
                std::string{"type,id,side,price,quantity\n"}
                + testCase.row + "\n"
            );

            const auto result = parse_order_events_csv(input);

            ASSERT_TRUE(std::holds_alternative<CsvParseError>(result));

            const auto& error = std::get<CsvParseError>(result);
            EXPECT_EQ(error.line, 2U);
            EXPECT_EQ(error.reason, testCase.reason);
        }
    }

}