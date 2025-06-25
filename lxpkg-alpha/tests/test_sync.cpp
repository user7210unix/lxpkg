#include <gtest/gtest/gtest.h>
#include "package/sync.hpp"
#include "../util.hpp"

TEST(SyncTest, SyncRepos) {
    std::vector<std::string> repos = {"https://github.com/LearnixOS/repo"};
    bool success = pkg::sync::SyncManager::syncRepos(repos, "/tmp/portage");
    EXPECT_TRUE(success); // Adjust based on network availability
}