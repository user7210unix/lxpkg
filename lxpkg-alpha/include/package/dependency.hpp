#pragma once
#include "package.hpp"
#include <set>
#include <vector>
#include <future>
#include "conflict.hpp"

struct Version {
    std::vector<int> parts;
    std::string suffix;
    Version(const std::string& versionStr);
    bool satisfies(const std::string& constraint) const;
    bool operator>=(const Version& other) const;
};

class DependencyManager {
public:
    enum class InstallChoice { Install, Skip, Stop };
    static bool resolveDependencies(const Package& pkg, const std::string& dbPath,
                                   std::set<std::string>& installed,
                                   std::vector<std::pair<std::string, std::string>>& toInstall,
                                   std::map<std::string, bool>& fileCache);
    static bool installDependencies(const std::vector<std::pair<std::string, std::string>>& toInstall,
                                   const Config& config, int maxJobs,
                                   ConflictManager::ConflictResolutionMode mode);
    static bool satisfiesConstraint(const std::string& version, const std::string& constraint);
    static InstallChoice confirmInstallation(const std::vector<std::pair<std::string, std::string>>& toInstall);
};