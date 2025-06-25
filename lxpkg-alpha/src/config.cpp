#include "config.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include "util.hpp"
#include <regex>

void Config::ensureConfigFile(const std::string& configPath) {
    if (!util::fileExists(configPath)) {
        util::logger->info("Creating default configuration file: {}", configPath);
        std::ofstream configFile(configPath);
        if (!configFile.is_open()) {
            util::logger->error("Failed to create configuration file: {}", configPath);
            return;
        }
        configFile << "[General]\n"
                   << "db_path=/var/db/lxpkg\n"
                   << "repo_path=/var/cache/lxpkg/repo\n"
                   << "repo_subdirs=core,wayland,extra,community\n"
                   << "make_opts=-j$(nproc)\n"
                   << "use_flags=gtk,wayland\n"
                   << "max_jobs=$(nproc)\n"
                   << "repos=https://github.com/LearnixOS/repo\n"
                   << "cache_dir=/var/cache/lxpkg\n";
        configFile.close();
        chmod(configPath.c_str(), 0644);
    }
}

Config::Config() {
    std::string configPath = "/etc/lxpkg.conf";
    ensureConfigFile(configPath);

    std::string cmd = "python3 ../python/config_parser.py " + configPath;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        util::logger->warn("Failed to read config via Python, using defaults");
        dbPath = "/var/db/lxpkg";
        repoPath = "/var/cache/lxpkg/repo";
        repoSubdirs = {"core", "wayland", "extra", "community"};
        makeOpts = "-j$(nproc)";
        useFlags = {"gtk", "wayland"};
        maxJobs = 4;
        repos = {"https://github.com/LearnixOS/repo"};
        cacheDir = "/var/cache/lxpkg";
        return;
    }

    std::stringstream buffer;
    std::string line;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) {
        buffer << buf;
    }
    pclose(pipe);

    std::string configStr = buffer.str();
    std::istringstream iss(configStr);
    while (std::getline(iss, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
            if (key == "db_path") dbPath = value;
            else if (key == "repo_path") repoPath = value;
            else if (key == "repo_subdirs") {
                std::istringstream subdirs(value);
                std::string subdir;
                while (std::getline(subdirs, subdir, ',')) {
                    repoSubdirs.push_back(subdir);
                }
            }
            else if (key == "make_opts") makeOpts = value;
            else if (key == "use_flags") {
                std::istringstream flags(value);
                std::string flag;
                while (std::getline(flags, flag, ',')) {
                    useFlags.insert(flag);
                }
            }
            else if (key == "max_jobs") {
                try {
                    maxJobs = std::stoi(value);
                } catch (...) {
                    maxJobs = std::stoi(util::captureCommandOutput("nproc"));
                }
            }
            else if (key == "repos") {
                std::istringstream repoList(value);
                std::string repo;
                while (std::getline(repoList, repo, ',')) {
                    repos.push_back(repo);
                }
            }
            else if (key == "cache_dir") cacheDir = value;
        }
    }
    if (makeOpts.find("$(nproc)") != std::string::npos) {
        std::string nproc = util::captureCommandOutput("nproc");
        makeOpts = std::regex_replace(makeOpts, std::regex("\\$\\(nproc\\)"), nproc);
    }
}