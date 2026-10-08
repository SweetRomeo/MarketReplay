//
// Created by berke on 8.10.2026.
//
#include "marketreplay/version.hpp"

#include <gtest/gtest.h>

TEST(VersionTest, ReturnsExpectedVersion)
{
    EXPECT_EQ(marketreplay::version(), "0.1.0");
}
