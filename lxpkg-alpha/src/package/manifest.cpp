#include "package/manifest.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <functional>
#include "util.hpp"

std::vector<std::string> ManifestGenerator::generateManifest(const std::string& dir) {
    std::vector<std::string> manifest;
    std::function<void(const std::string&)> scanDir = [&](const std::string& path) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return;
        struct dirent* entry;
        while ((entry = readdir(dir))) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            std::string fullPath = path + "/" + name;
            struct stat st;
            if (stat(fullPath.c_str(), &st) == 0) {
                if (S_ISREG(st.st_mode)) {
                    manifest.push_back(fullPath);
                } else if (S_ISDIR(st.st_mode)) {
                    scanDir(fullPath);
                }
            }
        }
        closedir(dir);
    };
    scanDir(dir);
    return manifest;
}