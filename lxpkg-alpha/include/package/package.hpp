#pragma once
#include <string>
#include <vector>
#include "config.hpp"
#include "../util.hpp"

class Package {
public:
    Package(const std::string& name_, const std::string& repoPath, const std::vector<std::string>& repoSubdirs);
    std::string getName() const { return name; }
    std::string getVersion() const { return version; }
    std::string getPackagePath() const { return packagePath; }
    std::string getBuildDir() const { return buildDir; }
    const std::vector<std::string>& getSources() const { return sources; }
    const std::vector<std::string>& getChecksums() const { return checksums; }
    const std::vector<std::pair<std::string, std::string>>& getDependencies() const { return dependencies; }
    const std::vector<std::string>& getBuildCommands() const { return buildCommands; }
    std::string getBuildSystem() const { return buildSystem; }

    bool readVersionFile();
    bool readSourcesFile();
    bool readChecksumsFile();
    bool readDependsFile();
    bool readBuildFile();

private:
    std::string name;
    std::string version;
    std::string packagePath;
    std::string buildDir;
    std::vector<std::string> sources;
    std::vector<std::string> checksums;
    std::vector<std::pair<std::string, std::string>> dependencies;
    std::vector<std::string> buildCommands;
    std::string buildSystem;
};