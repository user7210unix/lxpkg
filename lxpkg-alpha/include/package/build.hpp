#pragma once
#include <string>
#include <vector>
#include "package/package.hpp"

class BuildManager {
public:
    static bool build(const Package& pkg, const std::vector<std::string>& useFlags, bool clean);
};