#include "config.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <cstdlib>

Config::Config() {
    repoPath = "/var/db/lxpkg/repo";
    dbPath = "/var/db/lxpkg/installed";
    buildDir = "/tmp/lxpkg-build";
    makeFlags = "-j$(nproc)";
    useChroot = false;
    repoSubdirs = {"core", "extra", "wayland"};

    // Override with environment variables
    if (const char* env = std::getenv("LXPKG_REPO_PATH")) repoPath = env;
    if (const char* env = std::getenv("LXPKG_DB_PATH")) dbPath = env;
    if (const char* env = std::getenv("LXPKG_BUILD_DIR")) buildDir = env;
    if (const char* env = std::getenv("LXPKG_MAKEFLAGS")) makeFlags = env;
    if (const char* env = std::getenv("LXPKG_USE_CHROOT")) useChroot = std::string(env) == "1";

    // Load config file
    loadConfigFile("/etc/lxpkg.conf");

    // Ensure directories exist
    mkdir(dbPath.c_str(), 0755);
    mkdir(buildDir.c_str(), 0755);
}

void Config::loadConfigFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string key, value;
        ss >> key >> value;
        if (key == "REPO_PATH") repoPath = value;
        else if (key == "DB_PATH") dbPath = value;
        else if (key == "BUILD_DIR") buildDir = value;
        else if (key == "MAKEFLAGS") makeFlags = value;
        else if (key == "USE_CHROOT") useChroot = value == "1";
        else if (key == "REPO_SUBDIRS") {
            repoSubdirs.clear();
            std::string subdir;
            while (ss >> subdir) repoSubdirs.push_back(subdir);
        }
    }
    file.close();
}
