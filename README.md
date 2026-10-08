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