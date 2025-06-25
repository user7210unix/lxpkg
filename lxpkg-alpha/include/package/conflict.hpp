#pragma once
#include "package.hpp"
#include <vector>
#include <string>

class ConflictManager {
public:
    enum class ConflictResolutionMode { Prompt, Replace, Skip };
    static std::vector<std::pair<std::string, std::string>> checkConflicts(const Package& pkg, const std::vector<std::string>& files);
    static bool resolveConflicts(const Package& pkg, const std::vector<std::pair<std::string, std::string>>& conflicts,
                                 ConflictResolutionMode mode);
};