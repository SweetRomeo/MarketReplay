#include "marketreplay/version.hpp"

#include <iostream>

int main()
{
    std::cout << "MarketReplay "
              << marketreplay::version()
              << " ready\n";
    return 0;
}