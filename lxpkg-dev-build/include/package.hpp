#ifndef LXPKG_PACKAGE_HPP
#define LXPKG_PACKAGE_HPP

#include <string>
#include <vector>
#include <set>

class Config;
class Database;

class Package {
public:
    enum class InstallChoice { Install, Skip, Stop };

    Package(const std::string& name_, const std::string& repoPath, const std::vector<std::string>& repoSubdirs);
    
    bool readVersionFile(const std::string& repoPath, const std::vector<std::string>& repoSubdirs);
    bool readSourcesFile();
    bool readDependsFile();
    bool readBuildFile();
    bool fetchSource() const;
    bool build() const;
    bool tryFallbackBuild() const;
    bool install() const;
    bool resolveDependencies(const std::string& dbPath, std::set<std::string>& installed, 
                            std::vector<std::pair<std::string, std::string>>& toInstall) const;
    bool satisfiesConstraint(const std::string& version, const std::string& constraint) const;
    static InstallChoice confirmInstallation(const std::vector<std::pair<std::string, std::string>>& toInstall);
    bool remove() const;
    bool clean() const;
    std::vector<std::string> generateManifest(const std::string& dir) const;
    std::string getVersion() const { return version; }
    static void searchPackages(const std::string& repoPath, const std::vector<std::string>& repoSubdirs, 
                              const std::string& query, bool installedOnly);

private:
    std::string name;
    std::string version;
    std::string packagePath;
    std::string buildDir;
    std::string buildSystem;
    std::vector<std::string> sources;
    std::vector<std::string> buildCommands;
    std::vector<std::pair<std::string, std::string>> dependencies;
    std::string captureCommandOutput(const std::string& cmd) const;
    std::string findSourceDir() const;
};

#endif // LXPKG_PACKAGE_HPP
