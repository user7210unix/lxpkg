#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <getopt.h>  // Added for getopt_long
#include "config.hpp"
#include "database.hpp"
#include "package/package.hpp"
#include "package/source.hpp"
#include "package/build.hpp"
#include "package/install.hpp"
#include "package/dependency.hpp"
#include "package/search.hpp"
#include "package/sync.hpp"
#include "util.hpp"

int main(int argc, char* argv[]) {
    Config config;
    Database db("/var/lib/lxpkg/db");
    int opt;
    bool install = false, remove = false, sync = false, build = false, upgrade = false, clean = false, query = false, list = false;
    std::string pkgName, queryStr;

    static struct option long_options[] = {
        {"install", no_argument, nullptr, 'i'},
        {"remove", no_argument, nullptr, 'r'},
        {"sync", no_argument, nullptr, 's'},
        {"build", no_argument, nullptr, 'b'},
        {"upgrade", no_argument, nullptr, 'u'},
        {"clean", no_argument, nullptr, 'c'},
        {"query", no_argument, nullptr, 'q'},
        {"list", no_argument, nullptr, 'l'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    while ((opt = getopt_long(argc, argv, "irsbuclqh", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'i': install = true; pkgName = optarg ? optarg : ""; break;
            case 'r': remove = true; pkgName = optarg ? optarg : ""; break;
            case 's': sync = true; break;
            case 'b': build = true; pkgName = optarg ? optarg : ""; break;
            case 'u': upgrade = true; pkgName = optarg ? optarg : ""; break;
            case 'c': clean = true; pkgName = optarg ? optarg : ""; break;
            case 'q': query = true; queryStr = optarg ? optarg : ""; break;
            case 'l': list = true; break;
            case 'h':
                std::cout << "Usage: " << argv[0] << " [options] [package]\n"
                          << "Options:\n"
                          << "  -i, --install <pkg>  Install package\n"
                          << "  -r, --remove <pkg>   Remove package\n"
                          << "  -s, --sync           Sync repositories\n"
                          << "  -b, --build <pkg>    Build package\n"
                          << "  -u, --upgrade <pkg>  Upgrade package\n"
                          << "  -c, --clean <pkg>    Clean package\n"
                          << "  -q, --query <query>  Search packages\n"
                          << "  -l, --list           List installed packages\n"
                          << "  -h, --help           Show this help\n";
                return 0;
            default:
                std::cerr << "Usage: " << argv[0] << " [-irsbuclqh] [package]\n";
                return 1;
        }
    }

    if (sync) {
        if (!SyncManager::syncRepos(config.getRepos(), config.getRepoPath())) {
            util::logger->error("Repository sync failed");
            return 1;
        }
    }

    if (query) {
        SearchManager::searchPackages(config.getRepoPath(), config.getRepoSubdirs(), queryStr, false);
        return 0;
    }

    if (list) {
        std::set<std::string> installed = db.getInstalledPackages();
        for (const auto& pkg : installed) {
            std::cout << pkg << " " << db.getVersion(pkg) << "\n";
        }
        return 0;
    }

    if (remove) {
        Package pkg(pkgName, config.getRepoPath(), config.getRepoSubdirs());
        if (!InstallManager::remove(pkg)) {
            util::logger->error("Failed to remove {}", pkgName);
            return 1;
        }
        return 0;
    }

    if (clean) {
        Package pkg(pkgName, config.getRepoPath(), config.getRepoSubdirs());
        if (!InstallManager::clean(pkg)) {
            util::logger->error("Failed to clean {}", pkgName);
            return 1;
        }
        return 0;
    }

    if (upgrade) {
        if (!InstallManager::upgrade(pkgName, ConflictManager::ConflictResolutionMode::Prompt)) {
            util::logger->error("Failed to upgrade {}", pkgName);
            return 1;
        }
        return 0;
    }

    if (install || build) {
        Package pkg(pkgName, config.getRepoPath(), config.getRepoSubdirs());
        std::set<std::string> installed;
        std::vector<std::pair<std::string, std::string>> toInstall;
        std::map<std::string, bool> fileCache;
        if (!DependencyManager::resolveDependencies(pkg, "/var/lib/lxpkg/db", installed, toInstall, fileCache)) {
            util::logger->error("Failed to resolve dependencies for {}", pkgName);
            return 1;
        }

        if (DependencyManager::confirmInstallation(toInstall) != DependencyManager::InstallChoice::Install) {
            util::logger->info("Installation aborted");
            return 0;
        }

        if (!DependencyManager::installDependencies(toInstall, config, 4, ConflictManager::ConflictResolutionMode::Prompt)) {
            util::logger->error("Failed to install dependencies for {}", pkgName);
            return 1;
        }

        std::set<std::string> useFlagsSet = config.getUseFlags();
        std::vector<std::string> useFlags(useFlagsSet.begin(), useFlagsSet.end()); // Convert set to vector
        if ((build && !BuildManager::build(pkg, useFlags, true)) ||
            (install && !InstallManager::install(pkg, ConflictManager::ConflictResolutionMode::Prompt))) {
            util::logger->error("Failed to process package {}", pkgName);
            return 1;
        }
        util::logger->info("Package {} processed successfully", pkgName);
    }

    if (!(sync || query || list || remove || clean || upgrade || install || build)) {
        std::cerr << "No operation specified. Use -h for help.\n";
        return 1;
    }

    return 0;
}