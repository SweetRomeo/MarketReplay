# MarketReplay

C++ market data replay and limit order book reconstruction with correctness
tests and reproducible benchmarks.

## Current status

Initial project infrastructure: core library, CLI, and automated tests.
Market data replay and order book reconstruction are planned.

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

The test suite includes a core version unit test and a CLI smoke test.

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