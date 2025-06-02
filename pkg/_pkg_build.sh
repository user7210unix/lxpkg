#!/bin/sh
# Helper function to locate Cargo.toml
find_cargo_toml() {
    local dir="$1"
    local cargo_toml=$(find "$dir" -maxdepth 2 -type f -name "Cargo.toml" | head -n 1)
    if [ -n "$cargo_toml" ]; then
        dirname "$cargo_toml"
        return 0
    else
        warn "No Cargo.toml found in $dir or its subdirectories"
        return 1
    fi
}

# Detect build system
detect_build_system() {
    [ -f "Cargo.toml" ] && echo "cargo" && return
    [ -f "setup.py" ] || [ -f "requirements.txt" ] && echo "python" && return
    [ -f "meson.build" ] && echo "meson" && return
    [ -f "CMakeLists.txt" ] && echo "cmake" && return
    [ -f "autogen.sh" ] && echo "autotools-autogen" && return
    [ -f "configure" ] && echo "autotools" && return
    [ -f "Makefile" ] || [ -f "makefile" ] && echo "make" && return
    echo "unknown"
}

# Build package
pkg_build() {
    pkg_find "$1" || die "Package '$1' not found"
    pkg_verify "$1" # Calls pkg_find internally, so repo_dir is already set
    mkdir -p "$bld_dir/$1" || die "Failed to create build directory $bld_dir/$1"
    cd "$bld_dir/$1" || die "Failed to change to build directory $bld_dir/$1"
    
    log "Copying source files for $1 to $bld_dir/$1"
    # Copy all contents from source directory to build directory
    if ! cp -PRf "$src_dir/$1"/. "$bld_dir/$1" 2>&1; then
        warn "Failed to copy source files for $1"
        ls -la "$src_dir/$1" >&2
        die "Failed to copy source files for $1"
    fi
    
    local build_cwd="$bld_dir/$1" # The directory to change into for building
    if [ -f "$build_cwd/Cargo.toml" ]; then
        # If Cargo.toml is directly in the build_cwd, use it.
        # Otherwise, try to find it in subdirectories if it's not the root.
        if [ "$build_cwd" != "$src_dir/$1" ]; then # Only search if it's not the original source dir
             local cargo_found_dir=$(find_cargo_toml "$build_cwd")
             [ -n "$cargo_found_dir" ] && build_cwd="$cargo_found_dir"
        fi
    fi
    cd "$build_cwd" || die "Failed to change to build directory $build_cwd"
    
    log "Building $1 in $build_cwd..."
    local build_system=$(detect_build_system)
    log "Detected build system: $build_system for $1 in $PWD"
    case "$build_system" in
        cargo)
            log "Building Rust project $1 with Cargo"
            cargo build --release || die "Cargo build failed for $1"
            mkdir -p "$pkg_dir/$1/usr/bin"
            find target/release -maxdepth 1 -type f -executable -exec cp -f {} "$pkg_dir/$1/usr/bin/" \; || warn "Failed to copy binaries for $1"
            ;;
        python)
            log "Installing Python-based project $1"
            [ -f "setup.py" ] && {
                python3 setup.py install --prefix=/usr --root="$pkg_dir/$1" || die "Python setup.py install failed for $1"
            } || {
                [ -f "requirements.txt" ] && {
                    pip3 install -r requirements.txt --target="$pkg_dir/$1/usr/lib/python3/dist-packages" || warn "Failed to install Python dependencies for $1"
                }
                # Copy executable scripts from current directory (e.g., if it's a simple script)
                find . -maxdepth 1 -type f -executable -exec cp -f {} "$pkg_dir/$1/usr/bin/" \; 2>/dev/null || : # Suppress errors for no executables
            }
            ;;
        meson)
            meson setup build --prefix=/usr || die "Meson setup failed for $1"
            ninja -C build -j"$(nproc 2>/dev/null || echo 4)" || die "Ninja build failed for $1"
            ninja -C build install DESTDIR="$pkg_dir/$1" || die "Ninja install failed for $1"
            ;;
        cmake)
            mkdir -p build
            cd build || die "Failed to change to build directory"
            cmake .. -DCMAKE_INSTALL_PREFIX=/usr || die "CMake configure failed for $1"
            make -j"$(nproc 2>/dev/null || echo 4)" || die "Make failed for $1"
            make install DESTDIR="$pkg_dir/$1" || die "Make install failed for $1"
            cd .. || die "Failed to return from build directory"
            ;;
        autotools-autogen)
            ./autogen.sh || die "autogen.sh failed for $1"
            ./configure --prefix=/usr || die "Configure failed for $1"
            make -j"$(nproc 2>/dev/null || echo 4)" || die "Make failed for $1"
            make install DESTDIR="$pkg_dir/$1" || die "Make install failed for $1"
            ;;
        autotools)
            ./configure --prefix=/usr || die "Configure failed for $1"
            make -j"$(nproc 2>/dev/null || echo 4)" || die "Make failed for $1"
            make install DESTDIR="$pkg_dir/$1" || die "Make install failed for $1"
            ;;
        make)
            make -j"$(nproc 2>/dev/null || echo 4)" || die "Make failed for $1"
            make install DESTDIR="$pkg_dir/$1" || die "Make install failed for $1"
            ;;
        *)
            if [ -f "$repo_dir/build" ]; then
                log "Running custom build script for $1"
                sh -e "$repo_dir/build" "$pkg_dir/$1" || die "Custom build script failed for $1"
            else
                die "Unsupported build system for $1: $build_system. No custom build script found."
            fi
            ;;
    esac
    log "Build completed for $1"
}
