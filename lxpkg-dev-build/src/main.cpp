#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <iomanip>
#include "package.hpp"
#include "config.hpp"
#include "util.hpp"

// ANSI color codes
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[35m"
#define CYAN    "\033[36m"

// Thread-safe output function
std::mutex consoleMutex;
void print(const std::string& msg) {
    std::lock_guard<std::mutex> lock(consoleMutex);
    std::cout << msg << std::flush;
}

// Spinner animation function
void showSpinner(const std::atomic<bool>& running, const std::string& pkgName) {
    const char* spinner = "|/-\\";
    int i = 0;
    while (running) {
        print("\r" + std::string(CYAN) + "Processing " + BOLD + pkgName + RESET + " " + spinner[i++ % 4] + " ");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    print("\r" + std::string(50, ' ') + "\r");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print(std::string(RED) + "Usage: " + argv[0] + " <command> [package...]\n" + RESET);
        print("Commands: install, remove, search, clean\n");
        return 1;
    }

    std::string command = argv[1];
    std::vector<std::string> packages;
    for (int i = 2; i < argc; ++i) {
        packages.emplace_back(argv[i]);
    }

    Config config;
    std::string repoPath = config.getRepoPath();
    std::vector<std::string> repoSubdirs = config.getRepoSubdirs();

    if (command == "install") {
        if (packages.empty()) {
            print(std::string(RED) + "Error: No packages specified\n" + RESET);
            return 1;
        }
        for (const auto& pkgName : packages) {
            print("\n" + std::string(CYAN) + std::string(50, '=') + "\n" + 
                  CYAN + "Installing " + BOLD + pkgName + RESET + "\n" +
                  std::string(50, '-') + "\n");

            Package pkg(pkgName, repoPath, repoSubdirs);
            std::set<std::string> installed;
            std::vector<std::pair<std::string, std::string>> toInstall;

            // Start spinner for dependency resolution
            std::atomic<bool> running{true};
            std::thread spinner(showSpinner, std::ref(running), std::ref(pkgName));

            if (!pkg.resolveDependencies(config.getDbPath(), installed, toInstall)) {
                running = false;
                spinner.join();
                print(std::string(RED) + "Error: Failed to resolve dependencies for " + pkgName + "\n" + RESET);
                return 1;
            }

            // Pause spinner for dependency prompt
            running = false;
            spinner.join();
            Package::InstallChoice choice = Package::confirmInstallation(toInstall);
            if (choice == Package::InstallChoice::Stop) {
                print(std::string(YELLOW) + "Installation cancelled\n" + RESET);
                return 1;
            }

            if (choice == Package::InstallChoice::Install && !toInstall.empty()) {
                for (const auto& dep : toInstall) {
                    print(std::string(CYAN) + "Installing dependency " + BOLD + dep.first + RESET + "\n");
                    Package depPkg(dep.first, repoPath, repoSubdirs);
                    running = true;
                    spinner = std::thread(showSpinner, std::ref(running), std::ref(dep.first));
                    if (!depPkg.fetchSource()) {
                        running = false;
                        spinner.join();
                        print(std::string(RED) + "Error: Failed to fetch source for " + dep.first + "\n" + RESET);
                        return 1;
                    }
                    running = false; // Pause spinner for build output
                    spinner.join();
                    if (!depPkg.build()) {
                        print(std::string(RED) + "Error: Failed to build " + dep.first + "\n" + RESET);
                        return 1;
                    }
                    running = true;
                    spinner = std::thread(showSpinner, std::ref(running), std::ref(dep.first));
                    running = false; // Pause spinner for install output
                    spinner.join();
                    if (!depPkg.install()) {
                        print(std::string(RED) + "Error: Failed to install " + dep.first + "\n" + RESET);
                        return 1;
                    }
                    print(std::string(GREEN) + "Installed dependency " + BOLD + dep.first + RESET + "\n");
                }
            } else if (choice == Package::InstallChoice::Skip) {
                print(std::string(YELLOW) + "Warning: Skipping dependencies may cause installation failures\n" + RESET);
            }

            // Install the main package
            running = true;
            spinner = std::thread(showSpinner, std::ref(running), std::ref(pkgName));
            if (!pkg.fetchSource()) {
                running = false;
                spinner.join();
                print(std::string(RED) + "Error: Failed to fetch source for " + pkgName + "\n" + RESET);
                return 1;
            }
            running = false; // Pause spinner for build output
            spinner.join();
            if (!pkg.build()) {
                print(std::string(RED) + "Error: Failed to build " + pkgName + "\n" + RESET);
                return 1;
            }
            running = true;
            spinner = std::thread(showSpinner, std::ref(running), std::ref(pkgName));
            running = false; // Pause spinner for install output
            spinner.join();
            if (!pkg.install()) {
                print(std::string(RED) + "Error: Failed to install " + pkgName + "\n" + RESET);
                return 1;
            }
            print(std::string(GREEN) + "Successfully installed " + BOLD + pkgName + RESET + "\n" +
                  CYAN + std::string(50, '=') + "\n");
        }
    } else if (command == "remove") {
        if (packages.empty()) {
            print(std::string(RED) + "Error: No packages specified\n" + RESET);
            return 1;
        }
        for (const auto& pkgName : packages) {
            print("\n" + std::string(CYAN) + std::string(50, '=') + "\n" +
                  CYAN + "Removing " + BOLD + pkgName + RESET + "\n" +
                  std::string(50, '-') + "\n");

            Package pkg(pkgName, repoPath, repoSubdirs);
            std::atomic<bool> running{true};
            std::thread spinner(showSpinner, std::ref(running), std::ref(pkgName));

            if (!pkg.remove()) {
                running = false;
                spinner.join();
                print(std::string(RED) + "Error: Failed to remove " + pkgName + "\n" + RESET);
                return 1;
            }

            running = false;
            spinner.join();
            print(std::string(GREEN) + "Successfully removed " + BOLD + pkgName + RESET + "\n" +
                  CYAN + std::string(50, '=') + "\n");
        }
    } else if (command == "search") {
        print("\n" + std::string(CYAN) + "Searching packages\n" + RESET +
              std::string(50, '-') + "\n");
        Package::searchPackages(repoPath, repoSubdirs, packages.empty() ? "" : packages[0], false);
        print(std::string(CYAN) + std::string(50, '=') + "\n");
    } else if (command == "clean") {
        if (packages.empty()) {
            print(std::string(RED) + "Error: No packages specified\n" + RESET);
            return 1;
        }
        for (const auto& pkgName : packages) {
            print("\n" + std::string(CYAN) + std::string(50, '=') + "\n" +
                  CYAN + "Cleaning " + BOLD + pkgName + RESET + "\n" +
                  std::string(50, '-') + "\n");

            Package pkg(pkgName, repoPath, repoSubdirs);
            std::atomic<bool> running{true};
            std::thread spinner(showSpinner, std::ref(running), std::ref(pkgName));

            if (!pkg.clean()) {
                running = false;
                spinner.join();
                print(std::string(RED) + "Error: Failed to clean " + pkgName + "\n" + RESET);
                return 1;
            }

            running = false;
            spinner.join();
            print(std::string(GREEN) + "Successfully cleaned " + BOLD + pkgName + RESET + "\n" +
                  CYAN + std::string(50, '=') + "\n");
        }
    } else {
        print(std::string(RED) + "Unknown command: " + command + "\n" + RESET);
        return 1;
    }

    return 0;
}
