#include "package/search.hpp"
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include "util.hpp"
#include "database.hpp"

void SearchManager::searchPackages(const std::string& repoPath, const std::vector<std::string>& repoSubdirs,
                                   const std::string& query, bool installedOnly) {
    util::logger->info("Searching packages");
    std::cout << std::left << std::setw(20) << "Package" << std::setw(15) << "Version"
              << std::setw(50) << "Description" << "Repository\n";
    std::cout << std::string(95, '-') << "\n";

    Database db(Config().getDbPath());
    std::set<std::string> installedPackages = db.getInstalledPackages();

    for (const auto& subdir : repoSubdirs) {
        std::string dirPath = repoPath + "/" + subdir;
        DIR* dir = opendir(dirPath.c_str());
        if (!dir) {
            util::logger->error("Cannot open repository directory {}", dirPath);
            continue;
        }
        struct dirent* entry;
        while ((entry = readdir(dir))) {
            std::string pkgName = entry->d_name;
            if (pkgName == "." || pkgName == "..") continue;
            std::string pkgPath = dirPath + "/" + pkgName;
            std::string versionFile = pkgPath + "/version";
            if (!util::fileExists(versionFile)) continue;
            if (installedOnly && installedPackages.count(pkgName) == 0) continue;
            if (!query.empty() && pkgName.find(query) == std::string::npos) continue;

            std::string version;
            std::ifstream versionInFile(versionFile);
            if (versionInFile.is_open()) {
                std::getline(versionInFile, version);
                size_t spacePos = version.find(' ');
                if (spacePos != std::string::npos) {
                    version = version.substr(0, spacePos);
                }
                version.erase(std::remove_if(version.begin(), version.end(), ::isspace), version.end());
            } else {
                version = "unknown";
            }

            std::string description = "No description available";
            std::string descFile = pkgPath + "/description";
            if (util::fileExists(descFile)) {
                std::ifstream descInFile(descFile);
                if (descInFile.is_open()) {
                    std::getline(descInFile, description);
                }
            }

            std::cout << std::left << std::setw(20) << pkgName << std::setw(15) << version
                      << std::setw(50) << description << subdir << "\n";
        }
        closedir(dir);
    }
}