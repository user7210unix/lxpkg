#include "database.hpp"
#include "util.hpp"
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>

Database::Database(const std::string& dbPath_) : dbPath(dbPath_) {}

void Database::addPackage(const std::string& name, const std::string& version, const std::vector<std::string>& manifest) {
    std::string pkgDir = dbPath + "/" + name;
    if (!util::dirExists(pkgDir)) {
        mkdir(pkgDir.c_str(), 0755);
    }
    std::ofstream versionFile(pkgDir + "/version");
    if (versionFile.is_open()) {
        versionFile << version << "\n";
        versionFile.close();
    }
    std::ofstream manifestFile(pkgDir + "/manifest");
    if (manifestFile.is_open()) {
        for (const auto& file : manifest) {
            manifestFile << file << "\n";
        }
        manifestFile.close();
    }
}

std::vector<std::string> Database::getManifest(const std::string& name) {
    std::vector<std::string> manifest;
    std::ifstream manifestFile(dbPath + "/" + name + "/manifest");
    if (!manifestFile.is_open()) {
        return manifest;
    }
    std::string line;
    while (std::getline(manifestFile, line)) {
        if (!line.empty()) {
            manifest.push_back(line);
        }
    }
    manifestFile.close();
    return manifest;
}

void Database::removePackage(const std::string& name) {
    std::string pkgDir = dbPath + "/" + name;
    if (util::dirExists(pkgDir)) {
        system(("rm -rf " + pkgDir).c_str());
    }
}

std::set<std::string> Database::getInstalledPackages() {
    std::set<std::string> packages;
    DIR* dir = opendir(dbPath.c_str());
    if (!dir) {
        return packages;
    }
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        std::string name = entry->d_name;
        if (name != "." && name != ".." && util::fileExists(dbPath + "/" + name + "/version")) {
            packages.insert(name);
        }
    }
    closedir(dir);
    return packages;
}
