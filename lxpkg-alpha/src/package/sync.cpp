#include "package/sync.hpp"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "util.hpp"

using json = nlohmann::json;

bool SyncManager::syncRepos(const std::vector<std::string>& repos, const std::string& repoPath) {
    util::logger->info("Syncing repositories");
    std::filesystem::create_directories(repoPath);
    for (const auto& repo : repos) {
        std::string apiUrl = repo + "/releases/latest";
        std::string tmpFile = "/tmp/lxpkg_repo.json";
        std::string cmd = "curl -s -L -o \"" + tmpFile + "\" \"" + apiUrl + "\"";
        util::logger->info("Fetching release metadata: {}", cmd);
        if (system(cmd.c_str()) != 0) {
            util::logger->error("Failed to fetch release info for {}", repo);
            continue;
        }
        std::ifstream jsonFile(tmpFile);
        if (!jsonFile.is_open()) {
            util::logger->error("Failed to read release metadata for {}", repo);
            unlink(tmpFile.c_str());
            continue;
        }
        json releaseData;
        try {
            jsonFile >> releaseData;
        } catch (...) {
            util::logger->error("Invalid JSON in release metadata for {}", repo);
            unlink(tmpFile.c_str());
            continue;
        }
        unlink(tmpFile.c_str());

        std::vector<std::string> tarballs = {"core.tar.gz", "wayland.tar.gz", "extra.tar.gz"};
        for (const auto& tarball : tarballs) {
            std::string tarUrl;
            for (const auto& asset : releaseData["assets"]) {
                if (asset["name"] == tarball) {
                    tarUrl = asset["browser_download_url"];
                    break;
                }
            }
            if (tarUrl.empty()) {
                util::logger->warn("No {} found for {}", tarball, repo);
                continue;
            }

            std::string dest = "/tmp/" + tarball;
            cmd = "curl -L -o \"" + dest + "\" \"" + tarUrl + "\"";
            util::logger->info("Downloading: {}", cmd);
            if (system(cmd.c_str()) != 0) {
                util::logger->error("Failed to download {} for {}", tarball, repo);
                continue;
            }

            std::string extractPath = repoPath + "/" + tarball.substr(0, tarball.find(".tar.gz"));
            std::filesystem::remove_all(extractPath);
            std::filesystem::create_directories(extractPath);
            cmd = "tar -xzf \"" + dest + "\" -C \"" + extractPath + "\"";
            util::logger->info("Extracting {}: {}", tarball, cmd);
            if (system(cmd.c_str()) != 0) {
                util::logger->error("Failed to extract {} for {}", tarball, repo);
                unlink(dest.c_str());
                continue;
            }
            unlink(dest.c_str());
            util::logger->info("Successfully synced {}", tarball);
        }
    }
    util::logger->info("Repository sync completed");
    return true;
}