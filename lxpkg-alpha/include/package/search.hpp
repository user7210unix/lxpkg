#pragma once
#include "package.hpp"

class SearchManager {
public:
    static void searchPackages(const std::string& repoPath, const std::vector<std::string>& repoSubdirs,
                               const std::string& query, bool installedOnly);
};