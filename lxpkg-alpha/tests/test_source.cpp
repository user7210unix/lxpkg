#include <gtest/gtest/gtest.h>
#include "package/source.hpp"
#include "package/package.hpp"
#include "../util.hpp"

TEST(SourceTest, FindSourceDir) {
    pkg::Package pkg("testpkg", "/repo", {"core"});
    std::string sourceDir = pkg::source::SourceManager::findSourceDir(pkg.getName());
    EXPECT_TRUE(sourceDir.empty()); // Adjust based on mocked filesystem
}