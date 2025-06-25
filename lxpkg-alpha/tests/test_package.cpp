#include <gtest/gtest/gtest.h>
#include "package/package.hpp"
#include "../util.hpp"

TEST(PackageTest, Initialization) {
    pkg::Package pkg("testpkg", "/repo", {"core"});
    EXPECT_EQ(pkg.getName(), "testpkg");
    EXPECT_EQ(pkg.getBuildDir(), "/var/cache/pkg/testpkg");
}