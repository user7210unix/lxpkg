#include <gtest/gtest/gtest.h>
#include "package/dependency.hpp"
#include "../util.hpp"

TEST(DependencyTest, VersionParsing) {
    pkg::Version v1("1.2.3");
    pkg::Version v2("1.2.4");
    pkg::Version v3("1.2.3-r1");
    EXPECT_TRUE(v1.satisfies(">=1.2"));
    EXPECT_TRUE(v1.satisfies("=1.2.3"));
    EXPECT_TRUE(v2.satisfies("~1.2"));
    EXPECT_FALSE(v1.satisfies(">1.2.3"));
    EXPECT_TRUE(v3.satisfies(">=1.2.3"));
}