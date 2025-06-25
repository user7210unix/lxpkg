#include "package/source.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdlib>
#include "util.hpp"

bool SourceManager::verifyChecksum(const std::string& filePath, const std::string& expectedChecksum) {
    if (expectedChecksum.empty()) return true;
    std::string computed = util::computeSha256(filePath);
    if (computed != expectedChecksum) {
        util::logger->error("Checksum mismatch for {}: expected {}, got {}", filePath, expectedChecksum, computed);
        return false;
    }
    util::logger->info("Checksum verified for {}", filePath);
    return true;
}

bool SourceManager::fetchSource(const Package& pkg) {
    if (pkg.getSources().empty()) {
        util::logger->error("No sources defined for {}", pkg.getName());
        return false;
    }
    mkdir(pkg.getBuildDir().c_str(), 0755);
    for (size_t i = 0; i < pkg.getSources().size(); ++i) {
        std::string url = pkg.getSources()[i];
        std::string file = url.substr(url.find_last_of('/') + 1);
        std::string dest = pkg.getBuildDir() + "/" + file;
        std::string cmd = "curl -L -o \"" + dest + "\" \"" + url + "\"";
        util::logger->info("Fetching source for {}: {}", pkg.getName(), cmd);
        if (system(cmd.c_str()) != 0) {
            util::logger->error("Failed to download source for {}", pkg.getName());
            return false;
        }
        if (i < pkg.getChecksums().size() && !verifyChecksum(dest, pkg.getChecksums()[i])) {
            unlink(dest.c_str());
            return false;
        }
        cmd = "tar -xf \"" + dest + "\" -C \"" + pkg.getBuildDir() + "\"";
        util::logger->info("Extracting source: {}", cmd);
        if (system(cmd.c_str()) != 0) {
            util::logger->error("Failed to extract source for {}", pkg.getName());
            unlink(dest.c_str());
            return false;
        }
        unlink(dest.c_str());
    }
    return true;
}

std::string SourceManager::findSourceDir(const Package& pkg) {
    DIR* dir = opendir(pkg.getBuildDir().c_str());
    if (!dir) {
        util::logger->error("Cannot open build directory {}", pkg.getBuildDir());
        return "";
    }
    struct dirent* entry;
    std::string sourceDir;
    while ((entry = readdir(dir))) {
        std::string name = entry->d_name;
        if (entry->d_type == DT_DIR && name != "." && name != "..") {
            std::string path = pkg.getBuildDir() + "/" + name;
            sourceDir = path;
            break; // Assume first directory is the source
        }
    }
    closedir(dir);
    if (sourceDir.empty()) {
        util::logger->error("No valid source directory found in {}", pkg.getBuildDir());
    }
    return sourceDir;
}