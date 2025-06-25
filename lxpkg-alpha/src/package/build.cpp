#include "package/build.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>  // Added for chmod
#include "package/source.hpp"
#include "util.hpp"

bool BuildManager::build(const Package& pkg, const std::vector<std::string>& useFlags, bool clean) {
    util::logger->info("Building package {}", pkg.getName());
    std::string srcDir = "/tmp/lxpkg/" + pkg.getName();
    std::string buildScript = srcDir + "/build";

    if (!SourceManager::fetchSource(pkg)) {
        util::logger->error("Failed to fetch source for {}", pkg.getName());
        return false;
    }

    // Check if a build script exists in the source directory
    std::string packageBuildScript = srcDir + "/build";
    std::string buildInstructions;
    std::ifstream buildFileIn(packageBuildScript);
    if (buildFileIn.is_open()) {
        std::stringstream buffer;
        buffer << buildFileIn.rdbuf();
        buildInstructions = buffer.str();
        buildFileIn.close();
    } else {
        // Default build instructions if no build script is provided
        buildInstructions = "./configure --prefix=/usr\nmake\n";
    }

    std::ofstream buildFile(buildScript);
    if (!buildFile.is_open()) {
        util::logger->error("Failed to create build script for {}", pkg.getName());
        return false;
    }

    buildFile << "#!/bin/sh\n";
    buildFile << "cd \"" << srcDir << "\" || exit 1\n";
    for (const auto& flag : useFlags) {
        buildFile << "export USE_FLAGS=\"" << flag << " ${USE_FLAGS}\"\n";
    }
    buildFile << buildInstructions;
    buildFile << "make install DESTDIR=\"" << srcDir << "/dest\" || exit 1\n";
    buildFile.close();

    if (chmod(buildScript.c_str(), 0755) != 0) {
        util::logger->error("Failed to make build script executable for {}", pkg.getName());
        return false;
    }

    std::string cmd = clean ? "bwrap --bind / / --dev /dev --proc /proc --tmpfs /tmp --chdir " + srcDir + " " + buildScript
                            : buildScript;
    std::string output = util::captureCommandOutput(cmd);
    if (output.empty()) {
        util::logger->error("Build failed for {}", pkg.getName());
        return false;
    }

    util::logger->info("Build completed for {}", pkg.getName());
    return true;
}