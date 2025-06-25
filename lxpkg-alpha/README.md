# LXPKG - LXOS Package Manager

The official source-based package manager for LXOS, designed with minimalism and Unix philosophy in mind, inspired by KISS Linux. LXPKG is robust, extensible, and optimized for building and managing packages from source.

# Usage

lxpkg -i <package>       # Install package
lxpkg -r <package>       # Remove package
lxpkg -s                 # Sync repositories
lxpkg -b <package>       # Build package
lxpkg -u <package/world> # Upgrade package or system
lxpkg -c <package>       # Clean build directory
lxpkg -l                 # List installed packages
lxpkg -q <query>         # Search packages
lxpkg --help             # Show detailed help

# Examples

lxpkg -si firefox        # Sync and install firefox
lxpkg -sbu world         # Sync, build, and upgrade system
lxpkg -r nano            # Remove nano
lxpkg -q firefox         # Search for firefox

## Installation

1. Install dependencies:
   - g++, make, curl, wget, tar, bubblewrap, openssl, nlohmann-json3-dev
   - Python 3, colorama, tqdm
   - libspdlog-dev, libgtest-dev
2. Build and install:
   ```bash
   make
   sudo make install
