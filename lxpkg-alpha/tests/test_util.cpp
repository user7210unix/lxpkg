#include <gtest/gtest/gtest.h>
#include "../util.hpp"

TEST(UtilTest, FileExists) {
    EXPECT_FALSE(util::fileExists("/nonexistent"));
}