#pragma once
#include <string>
#include <mutex>
#include <spdlog/spdlog.h>
#include <regex>

namespace util {
    extern std::mutex consoleMutex;
    extern std::shared_ptr<spdlog::logger> logger;
    bool fileExists(const std::string& path);
    bool dirExists(const std::string& path);
    std::string captureCommandOutput(const std::string& cmd);
    std::string computeSha256(const std::string& filePath);
}