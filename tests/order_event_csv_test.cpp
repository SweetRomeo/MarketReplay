#include "marketreplay/order_event_csv.hpp"

#include <gtest/gtest.h>
#include <fstream>
#include <limits>
#include <streambuf>
#include <utility>
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


    TEST(OrderEventCsvTest, AcceptsBomCrLfWhitespaceAndFinalLineWithoutNewline)
    {
        std::istringstream input(
            "\xEF\xBB\xBF" "type,id,side,price,quantity\r\n"
            " \t\r\n"
            " ADD , 1 , SELL , 1050000 , 20 \r\n"
            " DELETE , 2 , , , ");
        const auto result = parse_order_events_csv(input);
        ASSERT_TRUE(std::holds_alternative<std::vector<OrderEvent>>(result));
        const auto& events = std::get<std::vector<OrderEvent>>(result);
        ASSERT_EQ(events.size(), 2U);
        const auto& order = std::get<AddEvent>(events[0]).order;
        EXPECT_EQ(order.id, 1U);
        EXPECT_EQ(order.side, Side::Sell);
        EXPECT_EQ(order.price, 1050000);
        EXPECT_EQ(order.quantity, 20U);
        EXPECT_EQ(std::get<DeleteEvent>(events[1]).id, 2U);
    }

    TEST(OrderEventCsvTest, AcceptsMaximumNumericValues)
    {
        std::istringstream input(
            "type,id,side,price,quantity\n"
            "ADD,18446744073709551615,BUY,9223372036854775807,4294967295\n");
        const auto result = parse_order_events_csv(input);
        ASSERT_TRUE(std::holds_alternative<std::vector<OrderEvent>>(result));
        const auto& events = std::get<std::vector<OrderEvent>>(result);
        ASSERT_EQ(events.size(), 1U);
        const auto& order = std::get<AddEvent>(events[0]).order;
        EXPECT_EQ(order.id, std::numeric_limits<OrderId>::max());
        EXPECT_EQ(order.side, Side::Buy);
        EXPECT_EQ(order.price, std::numeric_limits<Price>::max());
        EXPECT_EQ(order.quantity, std::numeric_limits<Quantity>::max());
    }

    TEST(OrderEventCsvTest, RejectsMalformedRowsWithoutPartialSuccess)
    {
        const char* rows[] = {
            "ADD,0,BUY,100,5", "ADD,-1,BUY,100,5", "ADD,+1,BUY,100,5",
            "ADD,18446744073709551616,BUY,100,5", "ADD,,BUY,100,5",
            "ADD,1,buy,100,5", "ADD,1,BUY,0,5", "ADD,1,BUY,-1,5",
            "ADD,1,BUY,9223372036854775808,5", "ADD,1,BUY,1.5,5",
            "ADD,1,BUY,100,0", "ADD,1,BUY,100,4294967296",
            "ADD,1,BUY,100,5abc", "ADD,1,BUY,100,", "ADD,1,BUY,100,+5",
            "ADD,1,BUY,100,-5", "ADD,1,BUY,100,5,extra",
            "ADD,1,\"BUY\",100,5", "# comment", "add,1,BUY,100,5",
            "CANCEL,1,,,", "CANCEL,+1,,,5", "DELETE,18446744073709551616,,,"
        };
        for (const auto* row : rows) {
            SCOPED_TRACE(row);
            std::istringstream input(
                std::string{"type,id,side,price,quantity\nADD,9,BUY,100,5\n\n"}
                + row + "\n");
            const auto result = parse_order_events_csv(input);
            ASSERT_TRUE(std::holds_alternative<CsvParseError>(result));
            const auto& error = std::get<CsvParseError>(result);
            EXPECT_EQ(error.line, 4U);
            EXPECT_FALSE(error.reason.empty());
        }
    }

    TEST(OrderEventCsvTest, EmptyInputReportsMissingHeader)
    {
        std::istringstream input;
        const auto result = parse_order_events_csv(input);
        ASSERT_TRUE(std::holds_alternative<CsvParseError>(result));
        EXPECT_EQ(std::get<CsvParseError>(result).line, 1U);
        EXPECT_EQ(std::get<CsvParseError>(result).reason, "Missing CSV header");
    }

    namespace {
        class FailingBuffer : public std::streambuf {
        public:
            explicit FailingBuffer(std::string prefix) : prefix_(std::move(prefix)) {}
        protected:
            int_type underflow() override {
                if (position_ == prefix_.size()) {
                    throw std::ios_base::failure("simulated read failure");
                }
                return traits_type::to_int_type(prefix_[position_]);
            }
            int_type uflow() override {
                const auto value = underflow();
                ++position_;
                return value;
            }
        private:
            std::string prefix_;
            std::size_t position_{};
        };
    }

    TEST(OrderEventCsvTest, ReportsHeaderAndLaterRowReadFailures)
    {
        for (const auto& prefix : {std::string{},
                 std::string{"type,id,side,price,quantity\nADD,1,BUY,100,5\n"}}) {
            FailingBuffer buffer(prefix);
            std::istream input(&buffer);
            const auto result = parse_order_events_csv(input);
            ASSERT_TRUE(std::holds_alternative<CsvParseError>(result));
            const auto& error = std::get<CsvParseError>(result);
            EXPECT_EQ(error.line, prefix.empty() ? 1U : 3U);
            EXPECT_EQ(error.reason, prefix.empty()
                ? "Failed to read CSV header" : "Failed to read CSV row");
        }
    }

    TEST(OrderEventCsvTest, SampleFixtureReplaysWithStateDependentErrors)
    {
        std::ifstream input(std::string{MARKETREPLAY_FIXTURE_DIR} + "/order_events.csv");
        ASSERT_TRUE(input.is_open());
        const auto parsed = parse_order_events_csv(input);
        ASSERT_TRUE(std::holds_alternative<std::vector<OrderEvent>>(parsed));
        OrderBook book;
        const auto results = replay_events(book, std::get<std::vector<OrderEvent>>(parsed));
        const std::vector<EventResult> expected{
            AddResult::Added, AddResult::Added, AddResult::Added,
            ReduceResult::Reduced, RemoveResult::Removed,
            AddResult::DuplicateId, ReduceResult::NotFound,
            ReduceResult::InvalidQuantity
        };
        EXPECT_EQ(results, expected);
        EXPECT_EQ(book.size(), 2U);
        ASSERT_TRUE(book.find(1).has_value());
        EXPECT_EQ(book.find(1)->quantity, 40U);
        ASSERT_TRUE(book.find(2).has_value());
        EXPECT_EQ(book.find(2)->quantity, 20U);
        EXPECT_FALSE(book.find(3).has_value());
        EXPECT_EQ(book.best_bid(), std::optional<Price>{1000000});
        EXPECT_EQ(book.best_ask(), std::optional<Price>{1050000});
    }

} // namespace marketreplay
