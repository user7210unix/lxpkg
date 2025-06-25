#pragma once
#include "package.hpp"
#include "conflict.hpp"

class InstallManager {
public:
    static bool install(const Package& pkg, ConflictManager::ConflictResolutionMode mode);
    static bool remove(const Package& pkg);
    static bool clean(const Package& pkg);
    static bool upgrade(const std::string& pkgName, ConflictManager::ConflictResolutionMode mode);
};