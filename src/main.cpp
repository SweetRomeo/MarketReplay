#include "marketreplay/order_event_csv.hpp"
#include "marketreplay/version.hpp"

#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

int main(int argc, char* argv[])
{
    using namespace marketreplay;

    if (argc == 1) {
        std::cout << "MarketReplay " << version() << " ready\n";
        return 0;
    }

    if (argc != 3 || std::string_view{argv[1]} != "replay") {
        std::cerr << "Usage: marketreplay replay <csv-path>\n";
        return 1;
    }

    std::ifstream input(argv[2]);

    if (!input.is_open()) {
        std::cerr << "Failed to open CSV file: " << argv[2] << '\n';
        return 1;
    }

    const auto parsed = parse_order_events_csv(input);

    if (const auto* error = std::get_if<CsvParseError>(&parsed)) {
        std::cerr << "CSV error at line " << error->line
                  << ": " << error->reason << '\n';
        return 1;
    }

    const auto& events = std::get<std::vector<OrderEvent>>(parsed);

    OrderBook book;
    const auto results = replay_events(book, events);

    std::size_t accepted = 0;

    for (const auto& result : results) {
        const bool success =
            result == EventResult{AddResult::Added}
            || result == EventResult{ReduceResult::Reduced}
            || result == EventResult{ReduceResult::Removed}
            || result == EventResult{RemoveResult::Removed};

        if (success) {
            ++accepted;
        }
    }

    std::cout << "Events: " << events.size() << '\n'
              << "Accepted: " << accepted << '\n'
              << "Rejected: " << results.size() - accepted << '\n'
              << "Remaining orders: " << book.size() << '\n'
              << "Prices: internal scaled integer units (scale "
              << price_scale << ")\n";

    const auto print_price = [](
        const char* label,
        const std::optional<Price>& price
    ) {
        std::cout << label;

        if (price.has_value()) {
            std::cout << *price;
        } else {
            std::cout << "N/A";
        }

        std::cout << '\n';
    };

    print_price("Best bid: ", book.best_bid());
    print_price("Best ask: ", book.best_ask());

    return 0;
}
