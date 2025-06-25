#include <gtest/gtest/gtest.h>
#include "package/conflict.hpp"
#include "package/package.hpp"
#include "../util.hpp"

TEST(ConflictTest, CheckConflicts) {
    pkg::Package pkg("testpkg", "/repo", {"core"});
    std::vector<std::string> files = {"/usr/bin/testpkg"};
    auto conflicts = pkg::conflict::ConflictManager::checkConflicts(files);
    EXPECT_TRUE(conflicts.empty()); // Adjust based on mocked database
}