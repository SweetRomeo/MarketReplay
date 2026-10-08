# MarketReplay
C++ market data replay and limit order book reconstruction with correctness tests and reproducible benchmarks.

## Build and test on Linux / WSL

Requirements: a C++20 compiler, CMake 3.20+, and Make.

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