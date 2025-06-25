#pragma once
#include "package.hpp"

class SourceManager {
public:
    static bool fetchSource(const Package& pkg);
    static std::string findSourceDir(const Package& pkg);
    static bool verifyChecksum(const std::string& filePath, const std::string& expectedChecksum);
};