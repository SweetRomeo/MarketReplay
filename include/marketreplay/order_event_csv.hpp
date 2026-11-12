#pragma once

#include "marketreplay/order_event.hpp"

#include <cstddef>
#include <istream>
#include <string>
#include <variant>
#include <vector>

namespace marketreplay {

    struct CsvParseError {
        std::size_t line;
        std::string reason;
    };

    using CsvParseResult = std::variant<
        std::vector<OrderEvent>,
        CsvParseError
    >;

    [[nodiscard]] CsvParseResult parse_order_events_csv(
        std::istream& input
    );

}
