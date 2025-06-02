#!/bin/sh
# Generate etcsums
pkg_etcsums() {
    local sums=$(mktemp)
    # Corrected path to find files under etc in the staging directory
    find "$pkg_dir/$1/etc" -type f 2>/dev/null | while read -r file; do
        file=${file#"$pkg_dir/$1/"} # Get relative path from pkg_dir/$1
        # Check if the file exists in the actual system root before calculating checksum
        [ -f "$LXPKG_ROOT/$file" ] && sha256sum "$LXPKG_ROOT/$file" | sed "s|$LXPKG_ROOT/||" >> "$sums"
    done
    # Ensure the destination directory exists for etcsums
    mkdir -p "$pkg_dir/$1/$pkg_db/$1"
    [ -s "$sums" ] && mv "$sums" "$pkg_dir/$1/$pkg_db/$1/etcsums" || rm -f "$sums"
}

# Handle /etc files
pkg_etc() {
    local pkg="$1" file="$2"
    # Check if a file with the same path exists in the system's /etc and its checksum matches
    if [ -f "$sys_db/$pkg/etcsums" ]; then
        local installed_checksum=$(grep "$file" "$sys_db/$pkg/etcsums" | awk '{print $1}' 2>/dev/null)
        local current_checksum=$(sha256sum "$LXPKG_ROOT/$file" | awk '{print $1}' 2>/dev/null)
        if [ "$installed_checksum" = "$current_checksum" ]; then
            log "Configuration file $file for $pkg is unchanged, skipping."
            return 0 # File is already identical, no action needed
        fi
    fi

    # If the file exists in the system and is different, move the new version to .lxpkgnew
    if [ -f "$LXPKG_ROOT/$file" ] && [ ! -L "$LXPKG_ROOT/$file" ]; then # Ensure it's a regular file and not a symlink
        warn "Existing configuration file $LXPKG_ROOT/$file for $pkg found. Moving to $LXPKG_ROOT/$file.lxpkgnew"
        mv "$LXPKG_ROOT/$file" "$LXPKG_ROOT/$file.lxpkgnew" || warn "Failed to move $LXPKG_ROOT/$file"
        return 0 # Indicate that the new file should be installed by the main loop
    fi
    return 1 # No existing file or it's a symlink, so proceed with normal installation
}

# Strip binaries
pkg_strip() {
    log "$1: Stripping binaries and libraries"
    [ "${LXPKG_STRIP:-1}" = "0" ] && return
    # Find ELF files and strip them.
    # Using 'file -i' to identify ELF executables/shared libraries is more robust than just path.
    find "$pkg_dir/$1" -type f -exec file -i {} + 2>/dev/null | grep -E 'x-executable|x-sharedlib' | awk -F': ' '{print $1}' | while read -r elf_file; do
        strip --strip-unneeded "$elf_file" 2>/dev/null || warn "Failed to strip $elf_file"
    done
}

# Check conflicts
pkg_conflicts() {
    log "$1: Checking for package conflicts"
    local current_pkg_manifest="$pkg_dir/$1/$pkg_db/$1/manifest"
    [ ! -f "$current_pkg_manifest" ] && { warn "Manifest not found for $1, cannot check for conflicts."; return 0; }

    while read -r file; do
        case $file in
            */ | var/db/lxpkg/installed/*) continue ;; # Skip directories and database files
        esac
        # Construct the full path in the target system
        local target_file="$LXPKG_ROOT/$file"

        # Check if the file already exists and is not a directory
        if [ -e "$target_file" ] && [ ! -d "$target_file" ]; then
            # Iterate through installed packages to find owners
            for installed_pkg_dir in "$sys_db/"*; do
                if [ -d "$installed_pkg_dir" ] && [ -f "$installed_pkg_dir/manifest" ]; then
                    local installed_pkg_name=$(basename "$installed_pkg_dir")
                    # Exclude the package being installed itself
                    if [ "$installed_pkg_name" = "$1" ]; then
                        continue
                    fi
                    # Check if the existing file is listed in another package's manifest
                    if grep -Fx "$file" "$installed_pkg_dir/manifest" >/dev/null; then
                        die "File $file from $1 conflicts with installed package $installed_pkg_name"
                    fi
                fi
            done
        fi
    done < "$current_pkg_manifest"
}


# Manage alternatives
pkg_alternatives() {
    if [ "$1" = "-" ]; then
        while read -r pkg path; do pkg_swap "$pkg" "$path"; done
    elif [ -n "$1" ]; then
        pkg_swap "$@"
    else
        set +f
        for alt in "$sys_ch/"*; do
            [ -e "$alt" ] || continue
            alt_name=$(basename "$alt")
            pkg=${alt_name%%>*}
            path=${alt_name#*>}
            path=${path//>/\/}
            printf '%s %s\n' "$pkg" "/$path"
        done
        set -f
    fi
}

# Swap alternatives
pkg_swap() {
    [ -d "$sys_db/$1" ] || die "'$1' not found"
    local conflict_name="$1${2//\//>}" # Transform / to > for filename
    [ -e "$sys_ch/$conflict_name" ] || die "Alternative '$1 $2' doesn't exist"
    
    if [ -e "$LXPKG_ROOT$2" ]; then
        # Find which package currently owns the file at $LXPKG_ROOT$2
        local other_pkg_manifest=$(grep -l "^$2$" "$sys_db"/*/manifest | head -n 1)
        local other_pkg=""
        if [ -n "$other_pkg_manifest" ]; then
            other_pkg=$(basename "$(dirname "$other_pkg_manifest")")
        fi

        if [ -n "$other_pkg" ]; then
            log "Swapping '$2' from '$other_pkg' to '$1'"
            local other_conflict_name="$other_pkg${2//\//>}"
            mv -f "$LXPKG_ROOT$2" "$sys_ch/$other_conflict_name" || die "Failed to move $LXPKG_ROOT$2 to $sys_ch/$other_conflict_name"
            # Update the manifest of the package that previously owned the file
            sed -i "s|^$2$|$cho_db/$other_conflict_name|" "$sys_db/$other_pkg/manifest" || die "Failed to update manifest for $other_pkg"
        else
            warn "File '$2' exists but isn't owned by any package. Moving it to alternative store anyway."
            mv -f "$LXPKG_ROOT$2" "$sys_ch/orphan>${2//\//>}" || die "Failed to move orphaned $LXPKG_ROOT$2"
        fi
    fi
    # Move the new alternative into place
    mv -f "$sys_ch/$conflict_name" "$LXPKG_ROOT$2" || die "Failed to move alternative $sys_ch/$conflict_name to $LXPKG_ROOT$2"
    # Update the manifest of the current package to reflect the installed path
    sed -i "s|^$cho_db/$conflict_name$|$2|" "$sys_db/$1/manifest" || die "Failed to update manifest for $1"
    log "Alternative $1 $2 swapped successfully."
}


# Install package
pkg_install() {
    local pkg="$1"
    pkg_find "$pkg" || die "Package '$pkg' not found"
    pkg_find_version "$pkg" # Sets repo_ver and repo_rel
    pkg_check_system "$pkg" && return 0
    local install_root="${BOOTSTRAP_LXPKG:-${LXPKG_ROOT:-/}}"
    
    # Validate version and release
    local clean_ver=$(printf '%s' "$repo_ver" | sed 's/[^0-9a-zA-Z.-]//g')
    local clean_rel=$(printf '%s' "$repo_rel" | sed 's/[^0-9a-zA-Z.-]//g')
    [ -z "$clean_ver" ] && die "Invalid version ($repo_ver) for $pkg"
    clean_rel=${clean_rel:-0}
    
    log "Preparing to install $c_cyan$pkg$c_reset version $c_green$repo_ver-$repo_rel$c_reset to $c_purple$install_root$c_reset"
    
    # Check disk space
    [ -z "$install_root" ] && die "Install root is empty"
    local disk_space=$(df -k "$install_root" | tail -1 | awk '{print $4}' 2>/dev/null || echo 0)
    [ "$disk_space" -lt 1024 ] && die "Insufficient disk space in $install_root (< 1MB available)"
    
    # Check and verify checksums
    if [ "$LXPKG_SKIP_CHECKSUMS" != "1" ] && pkg_checksums "$pkg"; then
        pkg_verify "$pkg" || {
            warn "Checksum verification failed for $pkg"
            prompt "Continue with installation of $pkg?"
        }
    else
        log "Skipping checksum verification for $pkg"
    fi
    
    # Attempt to use cached package or build
    log "Checking for cached package $pkg@$repo_ver-$repo_rel"
    if ! pkg_cache "$pkg"; then
        log "No cache found, building $pkg"
        pkg_source "$pkg" || die "Failed to download or extract sources for $pkg"
        pkg_build "$pkg" || die "Build failed for $pkg"
    fi
    
    # Change to package build directory for manifest generation
    cd "$pkg_dir/$pkg" || die "Failed to change to package build directory $pkg_dir/$pkg"
    mkdir -p "$pkg_db/$pkg" || die "Failed to create package database directory $pkg_db/$pkg"
    
    # Generate manifest if not exists
    [ -f "$pkg_db/$pkg/manifest" ] || {
        log "$pkg: Generating manifest"
        # Generate manifest from the *staged* files in pkg_dir/$pkg
        find . -type f -o -type l -o -type d | sed 's|^\./||' > "$pkg_db/$pkg/manifest" || die "Failed to generate manifest for $pkg"
    }
    
    # Validate manifest
    local file_count=$(wc -l < "$pkg_db/$pkg/manifest" 2>/dev/null || echo 0)
    [ "$file_count" -eq 0 ] && warn "Manifest is empty for $pkg. No files will be installed."
    
    # Fix dependencies - this was already done in pkg_resolve_deps,
    # but keeping it here as a sanity check or for direct calls to pkg_install
    # pkg_fix_deps "$pkg" # Consider if this is redundant or needed here.

    # Generate etcsums and strip binaries
    pkg_etcsums "$pkg"
    pkg_strip "$pkg"
    pkg_conflicts "$pkg"
    
    # Install files with progress bar
    log "Installing $c_cyan$pkg$c_reset version $c_green$repo_ver-$repo_rel$c_reset ($file_count files)"
    local i=0 dir_count=0 file_count_actual=0
    while read -r file; do
        i=$((i + 1))
        local percent=$((i * 100 / file_count))
        printf '\r%b%s %bInstalling %s: %3d%% [%s]%b' "$c_grey" "==>" "$c_grey" "$pkg" "$percent" "$(printf '%*s' "$((percent / 5))" | tr ' ' '#')" "$c_reset"
        
        local src_file="$pkg_dir/$pkg/$file"
        local dest_file="$install_root/$file"
        
        case $file in
            */)
                dir_count=$((dir_count + 1))
                mkdir -p "$dest_file" 2>/dev/null || warn "Failed to create directory $dest_file"
                ;;
            *)
                case "$file" in
                    etc/*[!/]) # Only handle /etc files that are actual files (not directories)
                        if pkg_etc "$pkg" "$file"; then
                            # If pkg_etc returns 0, it means the file was either identical or moved to .lxpkgnew
                            # In both cases, the new file should be installed (unless it was identical).
                            # The logic in pkg_etc should ensure that if it was identical, it skips.
                            # So, we still attempt to copy it here to ensure it's in place.
                            : # Handled by pkg_etc, proceed to copy
                        fi
                        ;;
                    var/db/lxpkg/installed/*)
                        # These are database files within the package staging area, not to be installed
                        continue
                        ;;
                esac

                mkdir -p "$(dirname "$dest_file")" || warn "Failed to create directory $(dirname "$dest_file")"
                
                if [ -f "$src_file" ] || [ -L "$src_file" ]; then
                    if cp -f "$src_file" "$dest_file" 2>/dev/null; then
                        file_count_actual=$((file_count_actual + 1))
                        [ "${LXPKG_VERBOSE:-0}" -eq 1 ] && log "Installed $src_file to $dest_file"
                    else
                        warn "Failed to install $src_file to $dest_file"
                    fi
                else
                    warn "Source file $src_file not found for $pkg, skipping."
                fi
                ;;
        esac
    done < "$pkg_db/$pkg/manifest"
    printf '\r%b%s %bInstalled %s: %3d%% [%-20s] (%d files, %d dirs)%b\n' "$c_grey" "==>" "$c_blue" "$pkg" 100 "$(printf '%*s' 20 | tr ' ' '#')" "$file_count_actual" "$dir_count" "$c_reset"

    # Copy the manifest and version file to the installed database
    log "Updating package database for $pkg"
    mkdir -p "$sys_db/$pkg"
    cp "$pkg_db/$pkg/manifest" "$sys_db/$pkg/manifest" || die "Failed to copy manifest for $pkg"
    printf '%s %s\n' "$repo_ver" "$repo_rel" > "$sys_db/$pkg/version" || die "Failed to write version for $pkg"
    
    log "Installed $c_cyan$pkg$c_reset successfully."
    
    # Clean up build and source directories for the installed package
    log "Cleaning up build and source directories for $pkg"
    rm -rf "$bld_dir/$pkg" "$src_dir/$pkg" || warn "Failed to clean up build/source directories for $pkg"
}
