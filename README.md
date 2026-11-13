# MarketReplay

C++ market data replay and limit order book reconstruction with correctness
tests and reproducible benchmarks.

## Current status

Core library with a single-instrument order book, in-memory event replay,
CSV event parsing, a startup CLI, and automated correctness tests.

## Build and test on Linux / WSL

Requirements:
- A C++20 compiler
- CMake 3.20 or newer
- Make
- Git
- Internet access during the initial configuration to download Google Test

```bash
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
./cmake-build-debug/marketreplay
```

Expected application output:

```text
MarketReplay 0.1.0 ready
```

The test suite covers the order model, order book, event replay, CSV parsing,
and a CLI smoke test.

## Build without tests

```bash
cmake -S . -B build -DBUILD_TESTING=OFF
cmake --build build
```

Google Test is not downloaded when testing is disabled.

## Order model

Each order contains an ID, side (Buy or Sell), price, and quantity.
The initial model represents orders for a single instrument.

Prices use signed 64-bit integers with a fixed scale of 10,000.
For example, 123.4567 is represented as 1234567.
The scale defines the storage precision, not an instrument's tick size.

Valid orders require a nonzero ID, a recognized side, a positive price,
and a positive quantity.

## Order book

The single-instrument OrderBook stores orders by ID.

Insertion returns Added, InvalidOrder, or DuplicateId.
Validation happens before duplicate detection.
Rejected insertions preserve existing orders and the book size.

Lookup returns an optional copy of the order, or std::nullopt when absent.
Changing the returned copy does not change the stored order.

size() and empty() report the number of stored orders.

## Order deletion and quantity reduction

- `remove(id)` deletes an order and returns `RemoveResult::Removed`.
  Missing IDs return `RemoveResult::NotFound`.
- `reduce(id, quantity)` subtracts the given quantity from an order;
  the argument is the amount to subtract, not the new remaining quantity.
- ID lookup takes precedence over quantity validation. Missing IDs return
  `ReduceResult::NotFound`, including when the quantity is zero.
- For existing orders, zero reductions and reductions exceeding the remaining
  quantity return `ReduceResult::InvalidQuantity` without changing state.
- A smaller positive reduction returns `ReduceResult::Reduced` and preserves
  the order's ID, side, and price.
- A reduction equal to the remaining quantity deletes the order and returns
  `ReduceResult::Removed`.
- Deleted IDs can be added again. Unrelated orders remain unchanged.

Quantity reduction provides shared behavior for future cancellation and
execution events. It does not perform matching or calculate trades or P&L.

## Best bid and best ask

- `best_bid()` returns the highest stored buy price.
- `best_ask()` returns the lowest stored sell price.
- Both return `std::optional<Price>`; a side with no orders returns
  `std::nullopt`.
- Queries do not modify orders or book size.
- Each query scans the stored orders: O(n) time and O(1) additional space.
- Results reflect the current book after additions, removals, and reductions.
- These queries return prices only; they do not select orders or execute trades.
- Locked books (best bid equals best ask) and crossed books
  (best bid exceeds best ask) remain permitted.

## In-memory order event replay

- `OrderEvent` represents an `AddEvent`, `CancelEvent`, or `DeleteEvent`.
- `apply_event(book, event)` applies one event and returns an `EventResult`
  containing the corresponding AddResult, ReduceResult, or RemoveResult.
- CancelEvent subtracts the specified quantity using OrderBook::reduce rules.
- `replay_events(book, events)` applies events in input order to the existing
  book and returns one result per event in the same order.
- Rejected events do not stop replay. Earlier successful changes remain applied.
- An empty event sequence returns an empty result vector without changing the book.
- Identical event sequences applied to identical initial states produce identical
  results and final book states.
- Replay does not reset the book, parse files, simulate timing, or execute trades.

## CSV order events

`parse_order_events_csv(std::istream&)` returns either the complete ordered
`std::vector<OrderEvent>` or `CsvParseError { line, reason }`. Error line numbers
are 1-based physical lines, including blank lines. Invalid input returns no
partial event sequence. Parsing never modifies an OrderBook.

The first line must be exactly `type,id,side,price,quantity` (an initial UTF-8
BOM is allowed). Each nonblank data row has exactly five comma-separated fields;
ASCII spaces and tabs around data fields are trimmed. LF, CRLF, trailing empty
fields, blank lines after the header, and a final line without newline are accepted.
Header-only input returns an empty sequence; empty input is a missing-header error.

```csv
type,id,side,price,quantity
ADD,1,BUY,1000000,50
ADD,2,SELL,1050000,20
CANCEL,1,,,10
DELETE,2,,,
```

- Types and sides are case-sensitive: ADD/CANCEL/DELETE and BUY/SELL.
- ADD requires all fields. CANCEL requires id and quantity with empty side/price.
  DELETE requires id with empty side/price/quantity.
- IDs are positive uint64 integers, prices positive int64 integers in existing
  scaled units (1000000 means 100.0000), and quantities positive uint32 integers.
  Signs, decimal points, numeric suffixes, zero, and overflow are rejected.
- Quoting, embedded commas, comments, and multiline fields are unsupported.
- Stream read failures are reported separately from successful EOF. Use the
  default stream exception mask; explicitly enabled stream exceptions propagate.
- Duplicate IDs, missing IDs, and excessive cancellations are state-dependent;
  parsing accepts them and `replay_events` reports their results.

`tests/fixtures/order_events.csv` is a reproducible mixed scenario, including
state-dependent rejections, exercised by the parser/replay integration test.
Open a file with `std::ifstream`, check it opened, parse, inspect the variant,
and pass the successful event vector to `replay_events(book, events)`.
The CLI supports CSV replay as described below.

## Replay a CSV from the CLI

Run from the repository root (quote file paths containing spaces):

```bash
./cmake-build-debug/marketreplay replay tests/fixtures/order_events.csv
```

Windows / Visual Studio:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
.\build\Debug\marketreplay.exe replay tests/fixtures/order_events.csv
ctest --test-dir build -C Debug --output-on-failure
```

Expected sample output:

```text
Events: 8
Accepted: 5
Rejected: 3
Remaining orders: 2
Prices: internal scaled integer units (scale 10000)
Best bid: 1000000
Best ask: 1050000
```

Every replay starts with an empty book. The complete CSV must parse successfully
before any events are applied. Accepted counts successful additions, reductions
(including exact cancellations that remove orders), and deletions. Duplicate IDs,
missing IDs and excessive cancellations count as rejected events; replay continues.
The sample intentionally contains three such rejections.

An empty book displays `N/A` for both prices; a missing side displays `N/A` for
that side. Prices are scaled integers, not decimal currency amounts.

Exit status is 0 for successful replay, including state-dependent rejections,
or for the existing no-argument startup message. Invalid arguments, file-open
failures and CSV errors return 1 and print an error to stderr with no summary
on stdout. CSV errors include the 1-based physical line number and reason.
Usage is exactly `marketreplay replay <csv-path>`.

CTest invokes the actual CLI to check summaries, exit codes and separate output
streams for valid, malformed, missing, empty and header-only files, argument
errors, one-sided books and removal of every order. These tests require no Python.
