#include "package/conflict.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "util.hpp"
#include "database.hpp"

std::vector<std::pair<std::string, std::string>> ConflictManager::checkConflicts(const Package& pkg,
                                                                                const std::vector<std::string>& files) {
    util::logger->info("Checking file conflicts for {}", pkg.getName());
    std::vector<std::pair<std::string, std::string>> conflicts;
    Database db(Config().getDbPath());
    for (const auto& file : files) {
        if (util::fileExists(file)) {
            std::string owner = db.findPackageOwningFile(file);
            if (!owner.empty() && owner != pkg.getName()) {
                conflicts.emplace_back(file, owner);
                util::logger->warn("Conflict: file {} owned by {}", file, owner);
            }
        }
    }
    return conflicts;
}

bool ConflictManager::resolveConflicts(const Package& pkg, const std::vector<std::pair<std::string, std::string>>& conflicts,
                                      ConflictResolutionMode mode) {
    util::logger->info("Resolving conflicts for {}", pkg.getName());
    for (const auto& [file, owner] : conflicts) {
        if (mode == ConflictResolutionMode::Skip) {
            util::logger->info("Skipping conflicting file: {}", file);
            return false;
        } else if (mode == ConflictResolutionMode::Replace) {
            util::logger->info("Replacing conflicting file: {}", file);
            std::filesystem::rename(file, file + ".bak");
        } else {
            std::cout << "Conflict: file " << file << " owned by " << owner << "\n";
            std::cout << "Options: [r]eplace, [s]kip, [b]ackup: ";
            std::string choice;
            std::getline(std::cin, choice);
            if (choice.empty() || choice[0] == 's' || choice[0] == 'S') {
                util::logger->info("Skipping conflicting file: {}", file);
                return false;
            } else if (choice[0] == 'r' || choice[0] == 'R') {
                util::logger->info("Replacing conflicting file: {}", file);
            } else if (choice[0] == 'b' || choice[0] == 'B') {
                util::logger->info("Backing up conflicting file: {}", file);
                std::filesystem::rename(file, file + ".bak");
            } else {
                util::logger->error("Invalid choice for conflict resolution");
                return false;
            }
        }
    }
    return true;
}