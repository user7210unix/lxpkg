#include <mutex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include <dirent.h>
#include <iomanip>
#include <algorithm> // Added for std::remove_if
#include <functional> // Added for std::function
#include <unistd.h> // Added for unlink
#include "package.hpp"
#include "config.hpp"
#include "util.hpp"
#include "database.hpp"

extern std::mutex consoleMutex;

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[35m"
#define CYAN    "\033[36m"
#define GREY    "\033[90m"
#define ITALIC  "\033[3m"

Package::Package(const std::string& name_, const std::string& repoPath, const std::vector<std::string>& repoSubdirs)
    : name(name_), buildDir("/tmp/lxpkg-build/" + name_) {
    for (const auto& subdir : repoSubdirs) {
        std::string path = repoPath + "/" + subdir + "/" + name;
        if (util::dirExists(path)) {
            packagePath = path;
            break;
        }
    }
    if (packagePath.empty()) {
        packagePath = repoPath + "/extra/" + name; // Default to extra
    }
    readVersionFile(repoPath, repoSubdirs);
    readSourcesFile();
    readDependsFile();
    readBuildFile();
}

bool Package::readVersionFile(const std::string& repoPath, const std::vector<std::string>& repoSubdirs) {
    std::ifstream inputFile(packagePath + "/version");
    if (!inputFile.is_open()) {
        std::cout << YELLOW << "Warning: No version file found for " << BOLD << name << RESET << YELLOW << ", using 'unknown'" << RESET << std::endl;
        version = "unknown";
        return true;
    }
    std::getline(inputFile, version);
    // Trim whitespace and take only the version number before any space
    size_t spacePos = version.find(' ');
    if (spacePos != std::string::npos) {
        version = version.substr(0, spacePos);
    }
    version.erase(std::remove_if(version.begin(), version.end(), ::isspace), version.end());
    inputFile.close();
    return true;
}

bool Package::readSourcesFile() {
    std::ifstream inputFile(packagePath + "/sources");
    if (!inputFile.is_open()) {
        std::cerr << RED << "Error: No sources file found for " << BOLD << name << RESET << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(inputFile, line)) {
        if (!line.empty()) {
            sources.emplace_back(line);
        }
    }
    inputFile.close();
    return true;
}

bool Package::readDependsFile() {
    std::ifstream inputFile(packagePath + "/depends");
    if (!inputFile.is_open()) {
        return true; // Dependencies are optional
    }
    std::string line;
    while (std::getline(inputFile, line)) {
        if (!line.empty()) {
            size_t pos = line.find(':');
            std::string depName = line.substr(0, pos);
            std::string constraint = (pos != std::string::npos) ? line.substr(pos + 1) : "";
            dependencies.emplace_back(depName, constraint);
        }
    }
    inputFile.close();
    return true;
}

bool Package::readBuildFile() {
    std::ifstream inputFile(packagePath + "/build");
    if (!inputFile.is_open()) {
        std::cerr << RED << "Error: No build file found for " << BOLD << name << RESET << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(inputFile, line)) {
        if (!line.empty() && line[0] != '#') {
            buildCommands.push_back(line);
        }
    }
    inputFile.close();
    if (buildCommands.empty()) {
        std::cerr << RED << "Error: Build file is empty or contains only comments for " << BOLD << name << RESET << std::endl;
        return false;
    }
    buildSystem = (buildCommands[0].find("configure") != std::string::npos || 
                   buildCommands[0].find("cmake") != std::string::npos || 
                   buildCommands[0].find("meson") != std::string::npos) ? "make" : "script";
    return true;
}

std::string Package::captureCommandOutput(const std::string& cmd) const {
    std::string result;
    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe) {
        std::cerr << RED << "Error: Failed to open pipe for command: " << cmd << RESET << std::endl;
        return result;
    }
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::lock_guard<std::mutex> lock(consoleMutex);
        std::cout << buffer << std::flush;
        result += buffer;
    }
    pclose(pipe);
    return result;
}

bool Package::fetchSource() const {
    std::cout << "\n" << CYAN << "==> Fetching source for " << BOLD << name << "-" << version << RESET << std::endl;
    if (sources.empty()) {
        std::cerr << RED << "Error: No sources defined for " << BOLD << name << RESET << std::endl;
        return false;
    }
    std::string url = sources[0];
    std::string file = url.substr(url.find_last_of('/') + 1);
    std::string dest = buildDir + "/source.tar";
    mkdir(buildDir.c_str(), 0755);
    std::string cmd = "wget -q --show-progress -O \"" + dest + "\" \"" + url + "\"";
    std::cout << GREY << ITALIC << "  -> Running: " << cmd << "\n" << RESET;
    std::string errorOutput = captureCommandOutput(cmd);
    if (system(cmd.c_str()) != 0) {
        std::cerr << RED << "Error: Failed to download source: " << errorOutput << RESET << std::endl;
        return false;
    }
    cmd = "tar -xJf \"" + dest + "\" -C \"" + buildDir + "\"";
    std::cout << GREY << ITALIC << "  -> Running: " << cmd << "\n" << RESET;
    errorOutput = captureCommandOutput(cmd);
    if (system(cmd.c_str()) != 0) {
        std::cerr << RED << "Error: Failed to extract source: " << errorOutput << RESET << std::endl;
        return false;
    }
    unlink(dest.c_str());
    return true;
}

std::string Package::findSourceDir() const {
    DIR* dir = opendir(buildDir.c_str());
    if (!dir) {
        std::cerr << RED << "Error: Cannot open build directory " << buildDir << RESET << std::endl;
        return "";
    }
    struct dirent* entry;
    std::string sourceDir;
    while ((entry = readdir(dir))) {
        std::string name = entry->d_name;
        if (entry->d_type == DT_DIR && name != "." && name != ".." && name != "install") {
            std::string path = buildDir + "/" + name;
            if (util::fileExists(path + "/configure") || util::fileExists(path + "/Makefile") ||
                util::fileExists(path + "/CMakeLists.txt") || util::fileExists(path + "/meson.build")) {
                sourceDir = path;
                break;
            }
        }
    }
    closedir(dir);
    if (sourceDir.empty()) {
        std::cerr << RED << "Error: No valid source directory found in " << buildDir << RESET << std::endl;
    }
    return sourceDir;
}

bool Package::build() const {
    std::cout << "\n" << CYAN << "==> Building " << BOLD << name << "-" << version << RESET << std::endl;
    std::string sourceDir = findSourceDir();
    if (sourceDir.empty()) {
        std::cerr << RED << "Error: No source directory found in " << buildDir << RESET << std::endl;
        return false;
    }
    if (buildSystem == "script") {
        std::cout << CYAN << "  -> Skipping compilation for script-based package" << RESET << std::endl;
        return true;
    }
    std::string scriptPath = buildDir + "/build.sh";
    std::ofstream script(scriptPath);
    if (!script.is_open()) {
        std::cerr << RED << "Error: Cannot create build script: " << scriptPath << RESET << std::endl;
        return false;
    }
    script << "#!/bin/sh\n";
    script << "set -xe\n";
    script << "cd \"" << sourceDir << "\" || exit 1\n";
    script << "export MAKEFLAGS=\"" << Config().getMakeFlags() << "\"\n";
    script << "dest=\"$1\"\n";
    for (const auto& cmd : buildCommands) {
        script << cmd << "\n";
    }
    script.close();
    if (chmod(scriptPath.c_str(), 0755) != 0) {
        std::cerr << RED << "Error: Failed to make build script executable" << RESET << std::endl;
        return false;
    }
    std::string destDir = buildDir + "/install";
    if (mkdir(destDir.c_str(), 0755) != 0 && errno != EEXIST) {
        std::cerr << RED << "Error: Failed to create install directory " << destDir << RESET << std::endl;
        return false;
    }
    std::string execCmd = scriptPath + " \"" + destDir + "\"";
    std::cout << GREY << ITALIC << "  -> Running: " << execCmd << "\n" << RESET;
    std::string errorOutput = captureCommandOutput(execCmd);
    if (system(execCmd.c_str()) != 0) {
        std::cerr << RED << "Build failed: \n" << errorOutput << RESET << std::endl;
        unlink(scriptPath.c_str());
        std::lock_guard<std::mutex> lock(consoleMutex);
        std::cout << YELLOW << "\nTry fallback build? [y/n]: " << RESET << std::flush;
        std::string input;
        std::getline(std::cin, input);
        if (input.empty() || input[0] == 'Y' || input[0] == 'y') {
            return tryFallbackBuild();
        }
        return false;
    }
    unlink(scriptPath.c_str());
    return true;
}

bool Package::tryFallbackBuild() const {
    std::cout << CYAN << "  -> Attempting fallback build for " << BOLD << name << RESET << std::endl;
    std::string sourceDir = findSourceDir();
    if (sourceDir.empty()) {
        std::cerr << RED << "Error: No source directory found for fallback build" << RESET << std::endl;
        return false;
    }
    std::string destDir = buildDir + "/install";
    std::string cmd = "cd \"" + sourceDir + "\" && make && make install DESTDIR=\"" + destDir + "\"";
    std::cout << GREY << ITALIC << "  -> Running: " << cmd << "\n" << RESET;
    std::string errorOutput = captureCommandOutput(cmd);
    if (system(cmd.c_str()) != 0) {
        std::cerr << RED << "Fallback build failed: \n" << errorOutput << RESET << std::endl;
        return false;
    }
    return true;
}

bool Package::install() const {
    std::cout << "\n" << CYAN << "==> Installing " << BOLD << name << "-" << version << RESET << std::endl;
    std::string destDir = buildDir + "/install";
    if (!util::dirExists(destDir)) {
        if (mkdir(destDir.c_str(), 0755) != 0 && errno != EEXIST) {
            std::cerr << RED << "Error: Failed to create installation directory " << destDir << RESET << std::endl;
            return false;
        }
    }
    if (buildSystem == "script") {
        std::string cmd = buildCommands.empty() ? "install -Dm755 " + name + " /usr/bin/" + name : buildCommands[0];
        std::cout << GREY << ITALIC << "  -> Running: " << cmd << "\n" << RESET;
        std::string errorOutput = captureCommandOutput(cmd);
        if (system(cmd.c_str()) != 0) {
            std::cerr << RED << "Installation failed for: " << name << ": " << errorOutput << RESET << std::endl;
            return tryFallbackBuild();
        }
        mkdir(destDir.c_str(), 0755);
        system(("mkdir -p " + destDir + "/usr/bin").c_str());
        system(("cp " + name + " " + destDir + "/usr/bin/" + name).c_str());
    } else {
        if (!util::dirExists(destDir)) {
            std::cerr << RED << "Error: Installation directory " << destDir << " not found after build" << RESET << std::endl;
            return false;
        }
        std::string cmd = "cp -r " + destDir + "/* /";
        std::cout << GREY << ITALIC << "  -> Running: " << cmd << "\n" << RESET;
        std::string errorOutput = captureCommandOutput(cmd);
        if (system(cmd.c_str()) != 0) {
            std::cerr << RED << "Installation failed: " << errorOutput << RESET << std::endl;
            return false;
        }
    }
    std::vector<std::string> manifest = generateManifest(destDir);
    Database db(Config().getDbPath());
    db.addPackage(name, version, manifest);
    std::cout << CYAN << "  -> Updating desktop database..." << RESET << std::endl;
    system("update-desktop-database /usr/share/applications 2>/dev/null || true");
    std::cout << CYAN << "  -> Updating icon cache..." << RESET << std::endl;
    system("gtk-update-icon-cache /usr/share/icons/hicolor 2>/dev/null || true");
    std::cout << GREEN << "Successfully installed " << BOLD << name << RESET << std::endl;
    return true;
}

bool Package::resolveDependencies(const std::string& dbPath, std::set<std::string>& installed, 
                                 std::vector<std::pair<std::string, std::string>>& toInstall) const {
    std::cout << CYAN << "  -> Checking dependencies for " << BOLD << name << RESET << std::endl;
    std::set<std::string> seen;
    
    // Check if the main package has a binary installed
    std::string binaryPath = "/usr/bin/" + name;
    if (util::fileExists(binaryPath)) {
        std::cout << CYAN << "    -> Package " << BOLD << name << RESET << CYAN 
                  << " binary found at " << binaryPath << ", marking as installed" << RESET << std::endl;
        installed.insert(name + ":" + (version.empty() ? "Any" : version));
    } else {
        // Check database for main package
        std::string versionFile = dbPath + "/" + name + "/version";
        if (util::fileExists(versionFile)) {
            std::ifstream file(versionFile);
            std::string installedVersion;
            std::getline(file, installedVersion);
            if (version.empty() || satisfiesConstraint(installedVersion, version)) {
                std::cout << CYAN << "    -> Package " << BOLD << name << RESET << CYAN 
                          << " found in database (version: " << installedVersion << ")" << RESET << std::endl;
                installed.insert(name + ":" + installedVersion);
            } else {
                toInstall.emplace_back(name, version);
                std::cout << CYAN << "    -> Package " << BOLD << name << RESET << CYAN 
                          << " needs installation (required: " << version << ", found: " << installedVersion << ")" << RESET << std::endl;
            }
        } else {
            toInstall.emplace_back(name, version);
            std::cout << CYAN << "    -> Package " << BOLD << name << RESET << CYAN 
                      << " needs installation" << RESET << std::endl;
        }
    }

    // Check dependencies
    for (const auto& dep : dependencies) {
        std::string depName = dep.first;
        std::string depConstraint = dep.second;
        std::string depKey = depName + ":" + depConstraint;
        std::cout << CYAN << "    -> Dependency: " << depName << " (" 
                  << (depConstraint.empty() ? "Any" : depConstraint) << ")" << RESET << std::endl;
        if (seen.count(depName)) {
            std::cerr << RED << "Error: Circular dependency detected: " << BOLD << depName << RESET << std::endl;
            return false;
        }
        seen.insert(depName);

        // Check if dependency binary exists
        std::string depBinaryPath = "/usr/bin/" + depName;
        if (util::fileExists(depBinaryPath)) {
            std::cout << CYAN << "    -> Dependency " << BOLD << depName << RESET << CYAN 
                      << " binary found at " << depBinaryPath << ", marking as installed" << RESET << std::endl;
            installed.insert(depKey);
            continue;
        }

        // Check database for dependency
        if (installed.count(depKey)) {
            std::cout << CYAN << "    -> Dependency " << BOLD << depName << RESET << CYAN 
                      << " already installed" << RESET << std::endl;
            continue;
        }
        std::string depVersionFile = dbPath + "/" + depName + "/version";
        if (util::fileExists(depVersionFile)) {
            std::ifstream file(depVersionFile);
            std::string installedVersion;
            std::getline(file, installedVersion);
            if (depConstraint.empty() || satisfiesConstraint(installedVersion, depConstraint)) {
                installed.insert(depKey);
                std::cout << CYAN << "    -> Dependency " << BOLD << depName << RESET << CYAN 
                          << " found in database (version: " << installedVersion << ")" << RESET << std::endl;
                continue;
            }
        }

        // Dependency not found, add to install list
        Package depPkg(depName, buildDir.substr(0, buildDir.find_last_of('/')), Config().getRepoSubdirs());
        toInstall.emplace_back(depName, depConstraint.empty() ? depPkg.getVersion() : depConstraint);
        std::cout << CYAN << "    -> Dependency " << BOLD << depName << RESET << CYAN 
                  << " needs installation" << RESET << std::endl;
        if (!depPkg.resolveDependencies(dbPath, installed, toInstall)) {
            return false;
        }
    }
    return true;
}

bool Package::satisfiesConstraint(const std::string& version, const std::string& constraint) const {
    if (constraint.empty()) {
        return true;
    }
    // Basic version comparison (e.g., "1.2.3" >= "1.2")
    size_t pos = constraint.find_first_of(">=<");
    if (pos == std::string::npos) {
        return version == constraint;
    }
    std::string op = constraint.substr(0, pos);
    std::string reqVersion = constraint.substr(pos);
    // Simplified comparison (implement proper version parsing if needed)
    return version >= reqVersion;
}

Package::InstallChoice Package::confirmInstallation(const std::vector<std::pair<std::string, std::string>>& toInstall) {
    if (toInstall.empty()) {
        return InstallChoice::Install;
    }
    std::cout << "\nThe following dependencies will be installed:\n";
    std::cout << std::string(50, '-') << "\n";
    std::cout << std::left << std::setw(20) << "Package" << "Version Constraint\n";
    std::cout << std::string(50, '-') << "\n";
    for (const auto& dep : toInstall) {
        std::cout << std::left << std::setw(20) << dep.first << dep.second << "\n";
    }
    std::cout << std::string(50, '-') << "\n";
    std::cout << "Proceed with installation? [y/n/s] (yes/no/skip dependencies): ";
    std::string input;
    std::getline(std::cin, input);
    if (input.empty() || input[0] == 'y' || input[0] == 'Y') {
        return InstallChoice::Install;
    } else if (input[0] == 's' || input[0] == 'S') {
        return InstallChoice::Skip;
    }
    return InstallChoice::Stop;
}

bool Package::remove() const {
    std::cout << "\n" << CYAN << "==> Removing " << BOLD << name << RESET << std::endl;
    Database db(Config().getDbPath());
    std::vector<std::string> manifest = db.getManifest(name);
    if (manifest.empty()) {
        std::cerr << RED << "Error: Package " << BOLD << name << RESET << " not found in database" << RESET << std::endl;
        return false;
    }
    for (const auto& file : manifest) {
        if (util::fileExists(file)) {
            std::cout << GREY << ITALIC << "  -> Removing: " << file << "\n" << RESET;
            unlink(file.c_str());
        }
    }
    db.removePackage(name);
    std::cout << CYAN << "  -> Updating desktop database..." << RESET << std::endl;
    system("update-desktop-database /usr/share/applications 2>/dev/null || true");
    std::cout << CYAN << "  -> Updating icon cache..." << RESET << std::endl;
    system("gtk-update-icon-cache /usr/share/icons/hicolor 2>/dev/null || true");
    return true;
}

bool Package::clean() const {
    std::cout << "\n" << CYAN << "==> Cleaning " << BOLD << name << RESET << std::endl;
    if (util::dirExists(buildDir)) {
        std::string cmd = "rm -rf \"" + buildDir + "\"";
        std::cout << GREY << ITALIC << "  -> Running: " << cmd << "\n" << RESET;
        std::string errorOutput = captureCommandOutput(cmd);
        if (system(cmd.c_str()) != 0) {
            std::cerr << RED << "Error: Failed to clean build directory: " << errorOutput << RESET << std::endl;
            return false;
        }
    }
    std::cout << GREEN << "Successfully cleaned " << BOLD << name << RESET << std::endl;
    return true;
}

std::vector<std::string> Package::generateManifest(const std::string& dir) const {
    std::vector<std::string> manifest;
    std::function<void(const std::string&)> scanDir = [&](const std::string& path) {
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            return;
        }
        struct dirent* entry;
        while ((entry = readdir(dir))) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") {
                continue;
            }
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

void Package::searchPackages(const std::string& repoPath, const std::vector<std::string>& repoSubdirs, 
                            const std::string& query, bool installedOnly) {
    std::cout << "\n" << CYAN << "Searching packages" << RESET << "\n" << std::string(50, '-') << "\n";
    std::cout << std::left << std::setw(20) << "Package" 
              << std::setw(15) << "Version" 
              << std::setw(50) << "Description" 
              << "Repository" << std::endl;
    std::cout << std::string(95, '-') << "\n";

    Database db(Config().getDbPath());
    std::set<std::string> installedPackages = db.getInstalledPackages();

    for (const auto& subdir : repoSubdirs) {
        std::string dirPath = repoPath + "/" + subdir;
        DIR* dir = opendir(dirPath.c_str());
        if (!dir) {
            std::cerr << RED << "Error: Cannot open repository directory " << dirPath << RESET << std::endl;
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(dir))) {
            std::string pkgName = entry->d_name;
            if (pkgName == "." || pkgName == "..") {
                continue;
            }
            std::string pkgPath = dirPath + "/" + pkgName;
            std::string versionFile = pkgPath + "/version";
            if (!util::fileExists(versionFile)) {
                continue; // Skip if not a valid package directory
            }
            if (installedOnly && installedPackages.count(pkgName) == 0) {
                continue; // Skip if not installed and installedOnly is true
            }
            if (!query.empty() && pkgName.find(query) == std::string::npos) {
                continue; // Skip if query doesn’t match package name
            }

            // Read version
            std::string version;
            std::ifstream versionInFile(versionFile);
            if (versionInFile.is_open()) {
                std::getline(versionInFile, version);
                size_t spacePos = version.find(' ');
                if (spacePos != std::string::npos) {
                    version = version.substr(0, spacePos);
                }
                version.erase(std::remove_if(version.begin(), version.end(), ::isspace), version.end());
                versionInFile.close();
            } else {
                version = "unknown";
            }

            // Read description (optional)
            std::string description = "No description available";
            std::string descFile = pkgPath + "/description";
            if (util::fileExists(descFile)) {
                std::ifstream descInFile(descFile);
                if (descInFile.is_open()) {
                    std::getline(descInFile, description);
                    descInFile.close();
                }
            }

            // Print package info
            std::cout << std::left << std::setw(20) << pkgName 
                      << std::setw(15) << version 
                      << std::setw(50) << description 
                      << subdir << std::endl;
        }
        closedir(dir);
    }
    std::cout << CYAN << std::string(95, '=') << "\n";
}
