#ifndef LXPKG_DATABASE_HPP
#define LXPKG_DATABASE_HPP

#include <string>
#include <vector>
#include <set>

class Database {
public:
    Database(const std::string& dbPath);
    void addPackage(const std::string& name, const std::string& version, const std::vector<std::string>& manifest);
    std::vector<std::string> getManifest(const std::string& name);
    void removePackage(const std::string& name);
    std::set<std::string> getInstalledPackages(); // Added for searchPackages
private:
    std::string dbPath;
};

#endif // LXPKG_DATABASE_HPP
