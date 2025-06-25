#include "package/package.hpp"
#include <fstream>
#include <algorithm>
#include "util.hpp"

Package::Package(const std::string& name_, const std::string& repoPath, const std::vector<std::string>& repoSubdirs)
    : name(name_), buildDir("/var/cache/lxpkg/" + name_) {
    for (const auto& subdir : repoSubdirs) {
        std::string path = repoPath + "/" + subdir + "/" + name;
        if (util::dirExists(path)) {
            packagePath = path;
            break;
        }
    }
    if (packagePath.empty()) {
        packagePath = repoPath + "/extra/" + name;
    }
    readVersionFile();
    readSourcesFile();
    readChecksumsFile();
    readDependsFile();
    readBuildFile();
}

bool Package::readVersionFile() {
    std::ifstream inputFile(packagePath + "/version");
    if (!inputFile.is_open()) {
        util::logger->warn("No version file found for {}, using 'unknown'", name);
        version = "unknown";
        return true;
    }
    std::getline(inputFile, version);
    size_t spacePos = version.find(' ');
    if (spacePos != std::string::npos) {
        version = version.substr(0, spacePos);
    }
    version.erase(std::remove_if(version.begin(), version.end(), ::isspace), version.end());
    return true;
}

bool Package::readSourcesFile() {
    std::ifstream inputFile(packagePath + "/sources");
    if (!inputFile.is_open()) {
        util::logger->error("No sources file found for {}", name);
        return false;
    }
    std::string line;
    while (std::getline(inputFile, line)) {
        if (!line.empty()) sources.push_back(line);
    }
    return true;
}

bool Package::readChecksumsFile() {
    std::ifstream inputFile(packagePath + "/checksums");
    if (!inputFile.is_open()) {
        util::logger->warn("No checksums file found for {}", name);
        return true; // Optional
    }
    std::string line;
    while (std::getline(inputFile, line)) {
        if (!line.empty()) checksums.push_back(line);
    }
    return true;
}

bool Package::readDependsFile() {
    std::ifstream inputFile(packagePath + "/depends");
    if (!inputFile.is_open()) return true; // Optional
    std::string line;
    while (std::getline(inputFile, line)) {
        if (!line.empty()) {
            size_t pos = line.find(':');
            std::string depName = line.substr(0, pos);
            std::string constraint = (pos != std::string::npos) ? line.substr(pos + 1) : "";
            dependencies.emplace_back(depName, constraint);
        }
    }
    return true;
}

bool Package::readBuildFile() {
    std::ifstream inputFile(packagePath + "/build");
    if (!inputFile.is_open()) {
        util::logger->error("No build file found for {}", name);
        return false;
    }
    std::string line;
    while (std::getline(inputFile, line)) {
        if (!line.empty() && line[0] != '#') {
            buildCommands.push_back(line);
        }
    }
    if (buildCommands.empty()) {
        util::logger->error("Build file is empty for {}", name);
        return false;
    }
    buildSystem = "script"; // All builds are POSIX shell scripts
    return true;
}