#pragma once
#include <string>
#include <vector>
#include "config.hpp"

class SyncManager {
public:
    static bool syncRepos(const std::vector<std::string>& repos, const std::string& repoPath);
};