#include "package/dependency.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <future>
#include "util.hpp"
#include "package/source.hpp"
#include "package/build.hpp"
#include "package/install.hpp"

Version::Version(const std::string& versionStr) {
    std::string numPart = versionStr;
    size_t suffixPos = versionStr.find_first_of("-");
    if (suffixPos != std::string::npos) {
        numPart = versionStr.substr(0, suffixPos);
        suffix = versionStr.substr(suffixPos);
    }
    std::istringstream iss(numPart);
    std::string part;
    while (std::getline(iss, part, '.')) {
        try {
            parts.push_back(std::stoi(part));
        } catch (...) {
            parts.push_back(0);
        }
    }
}

bool Version::operator>=(const Version& other) const {
    size_t maxLen = std::max(parts.size(), other.parts.size());
    for (size_t i = 0; i < maxLen; ++i) {
        int a = i < parts.size() ? parts[i] : 0;
        int b = i < other.parts.size() ? other.parts[i] : 0;
        if (a != b) return a > b;
    }
    return suffix >= other.suffix;
}

bool Version::operator==(const Version& other) const {
    return parts == other.parts && suffix == other.suffix;
}

bool Version::operator!=(const Version& other) const {
    return !(*this == other);
}

bool Version::operator<(const Version& other) const {
    size_t maxLen = std::max(parts.size(), other.parts.size());
    for (size_t i = 0; i < maxLen; ++i) {
        int a = i < parts.size() ? parts[i] : 0;
        int b = i < other.parts.size() ? other.parts[i] : 0;
        if (a != b) return a < b;
    }
    return suffix < other.suffix;
}

bool Version::satisfies(const std::string& constraint) const {
    if (constraint.empty()) return true;
    std::string op, reqVersion;
    size_t pos = constraint.find_first_of(">=<");
    if (pos == std::string::npos) {
        op = "=";
        reqVersion = constraint;
    } else {
        op = constraint.substr(0, pos);
        reqVersion = constraint.substr(pos);
    }
    Version req(reqVersion);
    if (op == ">=") return *this >= req;
    if (op == ">") return *this >= req && *this != req;
    if (op == "=") return *this == req;
    if (op == "<") return *this < req;
    if (op == "<=") return *this < req || *this == req;
    if (op == "~") {
        if (parts.size() < 2 || req.parts.size() < 2) return false;
        return parts[0] == req.parts[0] && parts[1] == req.parts[1];
    }
    return false;
}

bool DependencyManager::satisfiesConstraint(const std::string& version, const std::string& constraint) {
    Version v(version);
    return v.satisfies(constraint);
}

bool DependencyManager::resolveDependencies(const Package& pkg, const std::string& dbPath,
                                           std::set<std::string>& installed,
                                           std::vector<std::pair<std::string, std::string>>& toInstall,
                                           std::map<std::string, bool>& fileCache) {
    util::logger->info("Checking dependencies for {}", pkg.getName());
    std::set<std::string> seen;
    std::string binaryPath = "/usr/bin/" + pkg.getName();
    if (fileCache.find(binaryPath) != fileCache.end() ? fileCache[binaryPath] : (fileCache[binaryPath] = util::fileExists(binaryPath))) {
        util::logger->info("Package {} binary found at {}, marking as installed", pkg.getName(), binaryPath);
        installed.insert(pkg.getName() + ":" + (pkg.getVersion().empty() ? "Any" : pkg.getVersion()));
    } else {
        std::string versionFile = dbPath + "/" + pkg.getName() + "/version";
        if (fileCache.find(versionFile) != fileCache.end() ? fileCache[versionFile] : (fileCache[versionFile] = util::fileExists(versionFile))) {
            std::ifstream file(versionFile);
            std::string installedVersion;
            std::getline(file, installedVersion);
            if (pkg.getVersion().empty() || satisfiesConstraint(installedVersion, pkg.getVersion())) {
                util::logger->info("Package {} found in database (version: {})", pkg.getName(), installedVersion);
                installed.insert(pkg.getName() + ":" + installedVersion);
            } else {
                toInstall.emplace_back(pkg.getName(), pkg.getVersion());
                util::logger->info("Package {} needs installation (required: {}, found: {})", pkg.getName(), pkg.getVersion(), installedVersion);
            }
        } else {
            toInstall.emplace_back(pkg.getName(), pkg.getVersion());
            util::logger->info("Package {} needs installation", pkg.getName());
        }
    }

    for (const auto& dep : pkg.getDependencies()) {
        std::string depName = dep.first;
        std::string constraint = dep.second;
        std::string depKey = depName + ":" + constraint;
        if (seen.count(depName)) {
            util::logger->error("Circular dependency detected: {}", depName);
            return false;
        }
        seen.insert(depName);

        std::string depBinaryPath = "/usr/bin/" + depName;
        if (fileCache.find(depBinaryPath) != fileCache.end() ? fileCache[depBinaryPath] : (fileCache[depBinaryPath] = util::fileExists(depBinaryPath))) {
            util::logger->info("Dependency {} binary found at {}, marking as installed", depName, depBinaryPath);
            installed.insert(depKey);
            continue;
        }

        if (installed.count(depKey)) {
            util::logger->info("Dependency {} already installed", depName);
            continue;
        }
        std::string depVersionFile = dbPath + "/" + depName + "/version";
        if (fileCache.find(depVersionFile) != fileCache.end() ? fileCache[depVersionFile] : (fileCache[depVersionFile] = util::fileExists(depVersionFile))) {
            std::ifstream file(depVersionFile);
            std::string installedVersion;
            std::getline(file, installedVersion);
            if (constraint.empty() || satisfiesConstraint(installedVersion, constraint)) {
                util::logger->info("Dependency {} found in database (version: {})", depName, installedVersion);
                installed.insert(depKey);
                continue;
            }
        }

        Package depPkg(depName, Config().getRepoPath(), Config().getRepoSubdirs());
        toInstall.emplace_back(depName, constraint.empty() ? depPkg.getVersion() : constraint);
        util::logger->info("Dependency {} needs installation", depName);
        if (!resolveDependencies(depPkg, dbPath, installed, toInstall, fileCache)) {
            return false;
        }
    }
    return true;
}

bool DependencyManager::installDependencies(const std::vector<std::pair<std::string, std::string>>& toInstall,
                                           const Config& config, int maxJobs,
                                           ConflictManager::ConflictResolutionMode mode) {
    std::vector<std::future<bool>> futures;
    std::mutex installMutex;
    int activeJobs = 0;

    for (const auto& dep : toInstall) {
        while (activeJobs >= maxJobs) {
            for (auto it = futures.begin(); it != futures.end();) {
                if (it->wait_for(std::chrono::milliseconds(100)) == std::future_status::ready) {
                    if (!it->get()) {
                        util::logger->error("Failed to install dependency {}", dep.first);
                        return false;
                    }
                    it = futures.erase(it);
                    --activeJobs;
                } else {
                    ++it;
                }
            }
        }

        futures.push_back(std::async(std::launch::async, [&, dep, mode]() {
            Package depPkg(dep.first, config.getRepoPath(), config.getRepoSubdirs());
            {
                std::lock_guard<std::mutex> lock(installMutex);
                util::logger->info("Starting installation of dependency {}-{}", depPkg.getName(), depPkg.getVersion());
            }
            if (!SourceManager::fetchSource(depPkg)) {
                util::logger->error("Failed to fetch source for {}", depPkg.getName());
                return false;
            }
            if (!BuildManager::build(depPkg, config.getUseFlags(), true)) {
                util::logger->error("Failed to build {}", depPkg.getName());
                return false;
            }
            if (!InstallManager::install(depPkg, mode)) {
                util::logger->error("Failed to install {}", depPkg.getName());
                return false;
            }
            {
                std::lock_guard<std::mutex> lock(installMutex);
                util::logger->info("Successfully installed dependency {}", depPkg.getName());
            }
            return true;
        }));
        ++activeJobs;
    }

    for (auto& future : futures) {
        if (!future.get()) {
            util::logger->error("Dependency installation failed");
            return false;
        }
    }
    return true;
}

DependencyManager::InstallChoice DependencyManager::confirmInstallation(
    const std::vector<std::pair<std::string, std::string>>& toInstall) {
    if (toInstall.empty()) return InstallChoice::Install;
    util::logger->info("Dependencies to install:");
    for (const auto& dep : toInstall) {
        util::logger->info("  {} ({})", dep.first, dep.second);
    }
    std::cout << "Proceed? [y/n/s]: ";
    std::string input;
    std::getline(std::cin, input);
    if (input.empty() || input[0] == 'y' || input[0] == 'Y') return InstallChoice::Install;
    else if (input[0] == 's' || input[0] == 'S') return InstallChoice::Skip;
    return InstallChoice::Stop;
}