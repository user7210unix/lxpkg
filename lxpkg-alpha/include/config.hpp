#pragma once
#include <string>
#include <vector>
#include <set>

class Config {
public:
    Config();
    std::string getDbPath() const { return dbPath; }
    std::string getRepoPath() const { return repoPath; }
    std::vector<std::string> getRepoSubdirs() const { return repoSubdirs; }
    std::string getMakeOpts() const { return makeOpts; }
    std::set<std::string> getUseFlags() const { return useFlags; }
    int getMaxJobs() const { return maxJobs; }
    std::vector<std::string> getRepos() const { return repos; }
    std::string getCacheDir() const { return cacheDir; }
    static void ensureConfigFile(const std::string& configPath);

private:
    std::string dbPath;
    std::string repoPath;
    std::vector<std::string> repoSubdirs;
    std::string makeOpts;
    std::set<std::string> useFlags;
    int maxJobs;
    std::vector<std::string> repos;
    std::string cacheDir;
};