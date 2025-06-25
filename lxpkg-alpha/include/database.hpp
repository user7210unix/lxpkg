#pragma once
#include <string>
#include <vector>
#include <set>

class Database {
public:
    Database(const std::string& dbPath);
    void addPackage(const std::string& name, const std::string& version, const std::vector<std::string>& manifest);
    void removePackage(const std::string& name);
    std::vector<std::string> getManifest(const std::string& name) const;
    std::set<std::string> getInstalledPackages() const;
    std::string findPackageOwningFile(const std::string& filePath) const;
    std::string getVersion(const std::string& packageName) const;

private:
    std::string dbPath;
};