#include "package/install.hpp"
#include <filesystem>
#include <fstream>  // For manifest file reading
#include "database.hpp"
#include "util.hpp"
#include "package/source.hpp"
#include "package/build.hpp"

bool InstallManager::install(const Package& pkg, ConflictManager::ConflictResolutionMode mode) {
    // Read manifest from file instead of pkg.getManifest()
    std::string srcDir = "/tmp/lxpkg/" + pkg.getName();
    std::string manifestFile = srcDir + "/dest/manifest";
    std::vector<std::string> files;
    std::ifstream manifestIn(manifestFile);
    if (manifestIn.is_open()) {
        std::string line;
        while (std::getline(manifestIn, line)) {
            if (!line.empty()) files.push_back(line);
        }
        manifestIn.close();
    } else {
        util::logger->warn("No manifest file found for {} at {}", pkg.getName(), manifestFile);
    }

    auto conflicts = ConflictManager::checkConflicts(pkg, files);
    if (!conflicts.empty()) {
        if (!ConflictManager::resolveConflicts(pkg, conflicts, mode)) {
            util::logger->error("Failed to resolve conflicts for {}", pkg.getName());
            return false;
        }
    }

    for (const auto& file : files) {
        if (!std::filesystem::exists(file)) {
            util::logger->warn("File {} does not exist, skipping", file);
            continue;
        }
        std::string dest = "/usr" + file;
        std::filesystem::create_directories(std::filesystem::path(dest).parent_path());
        std::filesystem::copy_file(file, dest, std::filesystem::copy_options::overwrite_existing);
        util::logger->info("Installed {}", dest);
    }

    Database db("/var/lib/lxpkg/db");
    db.addPackage(pkg.getName(), pkg.getVersion(), files);
    util::logger->info("Package {} installed successfully", pkg.getName());
    return true;
}

bool InstallManager::remove(const Package& pkg) {
    // Read manifest from file instead of pkg.getManifest()
    std::string srcDir = "/tmp/lxpkg/" + pkg.getName();
    std::string manifestFile = srcDir + "/dest/manifest";
    std::vector<std::string> files;
    std::ifstream manifestIn(manifestFile);
    if (manifestIn.is_open()) {
        std::string line;
        while (std::getline(manifestIn, line)) {
            if (!line.empty()) files.push_back(line);
        }
        manifestIn.close();
    } else {
        util::logger->warn("No manifest file found for {} at {}", pkg.getName(), manifestFile);
    }

    for (const auto& file : files) {
        std::string path = "/usr" + file;
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
            util::logger->info("Removed {}", path);
        }
    }
    Database db("/var/lib/lxpkg/db");
    db.removePackage(pkg.getName());
    util::logger->info("Package {} removed successfully", pkg.getName());
    return true;
}

bool InstallManager::clean(const Package& pkg) {
    std::string srcDir = "/tmp/lxpkg/" + pkg.getName();
    if (std::filesystem::exists(srcDir)) {
        std::filesystem::remove_all(srcDir);
        util::logger->info("Cleaned source directory for {}", pkg.getName());
    }
    return true;
}

bool InstallManager::upgrade(const std::string& pkgName, ConflictManager::ConflictResolutionMode mode) {
    Config config;
    Package pkg(pkgName, config.getRepoPath(), config.getRepoSubdirs());
    Database db("/var/lib/lxpkg/db");
    std::string installedVersion = db.getVersion(pkgName);
    if (installedVersion.empty()) {
        util::logger->info("Package {} is not installed, installing instead", pkgName);
    } else if (installedVersion == pkg.getVersion()) {
        util::logger->info("Package {} is already up-to-date", pkgName);
        return true;
    } else {
        util::logger->info("Upgrading {} from {} to {}", pkgName, installedVersion, pkg.getVersion());
        if (!remove(Package(pkgName, config.getRepoPath(), config.getRepoSubdirs()))) {
            util::logger->error("Failed to remove existing package {}", pkgName);
            return false;
        }
    }
    std::set<std::string> useFlagsSet = config.getUseFlags();
    std::vector<std::string> useFlags(useFlagsSet.begin(), useFlagsSet.end()); // Convert set to vector
    if (!SourceManager::fetchSource(pkg) ||
        !BuildManager::build(pkg, useFlags, true) ||
        !install(pkg, mode)) {
        util::logger->error("Failed to upgrade {}", pkgName);
        return false;
    }
    util::logger->info("Package {} upgraded successfully", pkgName);
    return true;
}