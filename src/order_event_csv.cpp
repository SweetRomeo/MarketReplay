//
// Created by berke on 28.10.2026.
//

#include "marketreplay/order_event_csv.hpp"

#include <charconv>
#include <string>
#include <system_error>
#include <vector>

namespace marketreplay {

    namespace {

        template <typename T>
        bool parse_positive_integer(const std::string& text, T& value)
        {
            if (text.empty()) {
                return false;
            }

            // Reject signs and characters other than decimal digits.
            if (text.find_first_not_of("0123456789")
                != std::string::npos) {
                return false;
            }

            T parsed{};
            const auto* begin = text.data();
            const auto* end = begin + text.size();

            const auto [ptr, ec] = std::from_chars(begin, end, parsed);

            if (ec != std::errc{} || ptr != end || parsed == 0) {
                return false;
            }

            value = parsed;
            return true;
        }

        std::string trim_ascii(const std::string& value)
        {
            const auto first = value.find_first_not_of(" \t");

            if (first == std::string::npos) {
                return {};
            }

            const auto last = value.find_last_not_of(" \t");

            return value.substr(first, last - first + 1);
        }

        std::vector<std::string> split_fields(const std::string& line)
        {
            std::vector<std::string> fields;
            std::size_t start = 0;

            while (true) {
                const auto comma = line.find(',', start);

                fields.push_back(
                    trim_ascii(line.substr(start, comma - start))
                );

                if (comma == std::string::npos) {
                    break;
                }

                start = comma + 1;
            }

            return fields;
        }

    } // namespace

    CsvParseResult parse_order_events_csv(std::istream& input)
    {
        std::string line;

        if (!std::getline(input, line)) {
            if (input.bad() || !input.eof()) {
                return CsvParseError{1, "Failed to read CSV header"};
            }

            return CsvParseError{1, "Missing CSV header"};
        }

        // Handle Windows CRLF.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Handle an optional UTF-8 BOM.
        if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
            line.erase(0, 3);
        }

        if (line != "type,id,side,price,quantity") {
            return CsvParseError{1, "Invalid CSV header"};
        }

        std::vector<OrderEvent> events;
        std::size_t lineNumber = 1;

        while (std::getline(input, line)) {
            ++lineNumber;

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (trim_ascii(line).empty()) {
                continue;
            }

            if (line.find('"') != std::string::npos) {
                return CsvParseError{
                    lineNumber,
                    "Quoted fields are not supported"
                };
            }

            const auto fields = split_fields(line);

            if (fields.size() != 5) {
                return CsvParseError{
                    lineNumber,
                    "Expected exactly 5 fields"
                };
            }

            if (fields[0] == "ADD") {
                Order order{};

                if (!parse_positive_integer(fields[1], order.id)) {
                    return CsvParseError{
                        lineNumber,
                        "Invalid ADD id"
                    };
                }

                if (fields[2] == "BUY") {
                    order.side = Side::Buy;
                } else if (fields[2] == "SELL") {
                    order.side = Side::Sell;
                } else {
                    return CsvParseError{
                        lineNumber,
                        "Invalid ADD side"
                    };
                }

                if (!parse_positive_integer(fields[3], order.price)) {
                    return CsvParseError{
                        lineNumber,
                        "Invalid ADD price"
                    };
                }

                if (!parse_positive_integer(fields[4], order.quantity)) {
                    return CsvParseError{
                        lineNumber,
                        "Invalid ADD quantity"
                    };
                }

                events.emplace_back(AddEvent{order});
                continue;
            }

            if (fields[0] == "CANCEL") {
                CancelEvent event{};

                if (!parse_positive_integer(fields[1], event.id)) {
                    return CsvParseError{
                        lineNumber,
                        "Invalid CANCEL id"
                    };
                }

                if (!fields[2].empty() || !fields[3].empty()) {
                    return CsvParseError{
                        lineNumber,
                        "CANCEL side and price must be empty"
                    };
                }

                if (!parse_positive_integer(fields[4], event.quantity)) {
                    return CsvParseError{
                        lineNumber,
                        "Invalid CANCEL quantity"
                    };
                }

                events.emplace_back(event);
                continue;
            }

            if (fields[0] == "DELETE") {
                DeleteEvent event{};

                if (!parse_positive_integer(fields[1], event.id)) {
                    return CsvParseError{
                        lineNumber,
                        "Invalid DELETE id"
                    };
                }

                if (!fields[2].empty()
                    || !fields[3].empty()
                    || !fields[4].empty()) {
                    return CsvParseError{
                        lineNumber,
                        "DELETE side, price and quantity must be empty"
                    };
                }

                events.emplace_back(event);
                continue;
            }

            return CsvParseError{
                lineNumber,
                "Unknown event type"
            };
        }

        if (input.bad() || !input.eof()) {
            return CsvParseError{
                lineNumber + 1,
                "Failed to read CSV row"
            };
        }

        return events;
    }

} // namespace marketreplay
