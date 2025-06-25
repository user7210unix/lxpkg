#include "database.hpp"
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include "util.hpp"

Database::Database(const std::string& dbPath_) : dbPath(dbPath_) {
    mkdir(dbPath.c_str(), 0755);
}

void Database::addPackage(const std::string& name, const std::string& version, const std::vector<std::string>& manifest) {
    std::string pkgDir = dbPath + "/" + name;
    mkdir(pkgDir.c_str(), 0755);
    std::ofstream versionFile(pkgDir + "/version");
    if (versionFile.is_open()) {
        versionFile << version << "\n";
    }
    std::ofstream manifestFile(pkgDir + "/manifest");
    if (manifestFile.is_open()) {
        for (const auto& file : manifest) {
            manifestFile << file << "\n";
        }
    }
    util::logger->info("Added package {} to database", name);
}

void Database::removePackage(const std::string& name) {
    std::string pkgDir = dbPath + "/" + name;
    if (util::fileExists(pkgDir)) {
        util::captureCommandOutput("rm -rf " + pkgDir);
    }
    util::logger->info("Removed package {} from database", name);
}

std::vector<std::string> Database::getManifest(const std::string& name) const {
    std::vector<std::string> manifest;
    std::string manifestFile = dbPath + "/" + name + "/manifest";
    std::ifstream file(manifestFile);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) manifest.push_back(line);
        }
    }
    return manifest;
}

std::set<std::string> Database::getInstalledPackages() const {
    std::set<std::string> packages;
    DIR* dir = opendir(dbPath.c_str());
    if (!dir) return packages;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            packages.insert(name);
        }
    }
    closedir(dir);
    return packages;
}

std::string Database::findPackageOwningFile(const std::string& filePath) const {
    DIR* dir = opendir(dbPath.c_str());
    if (!dir) return "";
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        std::string pkgName = entry->d_name;
        if (pkgName == "." || pkgName == "..") continue;
        std::vector<std::string> manifest = getManifest(pkgName);
        for (const auto& file : manifest) {
            if (file == filePath) {
                closedir(dir);
                return pkgName;
            }
        }
    }
    closedir(dir);
    return "";
}

std::string Database::getVersion(const std::string& packageName) const {
    std::string versionFile = dbPath + "/" + packageName + "/version";
    if (!util::fileExists(versionFile)) return "";
    std::ifstream file(versionFile);
    std::string version;
    std::getline(file, version);
    return version;
}