#!/bin/sh
# Remove package
pkg_remove() {
    local pkg="$1"
    [ -d "$sys_db/$pkg" ] || die "Package '$pkg' not found in system"
    [ -f "$sys_db/$pkg/manifest" ] || warn "No manifest found for $pkg, proceeding with database cleanup"
    log "Removing $pkg from system..."
    # Ensure post-remove script exists before trying to run it
    if [ -f "/usr/src/lxpkg/repo/extra/$pkg/post-remove" ]; then # Assuming extra packages are in /usr/src/lxpkg/repo/extra
        log "Running post-remove script for $pkg"
        sh -e "/usr/src/lxpkg/repo/extra/$pkg/post-remove" "$LXPKG_ROOT/" || warn "Post-remove script failed for $pkg"
    fi
    local removed=0
    if [ -f "$sys_db/$pkg/manifest" ]; then
        log "Processing manifest at $sys_db/$pkg/manifest"
        # Read manifest in reverse order to remove files before directories
        tac "$sys_db/$pkg/manifest" | while read -r file; do
            case $file in
                */) continue ;; # Skip directories for now, handle them separately
                *)
                    local dest_file="$LXPKG_ROOT/$file"
                    if [ -f "$dest_file" ] || [ -L "$dest_file" ]; then
                        log "Removing $dest_file"
                        rm -f "$dest_file" || warn "Failed to remove $dest_file"
                        removed=$((removed + 1))
                    fi
                    ;;
            esac
        done
    fi
    
    # Attempt to remove directories after files
    if [ -f "$sys_db/$pkg/manifest" ]; then
        tac "$sys_db/$pkg/manifest" | while read -r file; do
            case $file in
                */) # Only process directories
                    local dir="$LXPKG_ROOT/$file"
                    if [ -d "$dir" ]; then
                        # Try to remove empty directories. Errors are okay if they are not empty.
                        rmdir "$dir" 2>/dev/null || :
                    fi
                    ;;
            esac
        done
    fi

    # Cleanup any binaries that might have been symlinked or placed directly
    # This part should ideally be covered by the manifest, but as a fallback:
    for bin_path in /usr/local/bin /usr/bin; do
        if [ -f "$bin_path/$pkg" ] && [ -L "$bin_path/$pkg" ] && [ "$(readlink -f "$bin_path/$pkg")" =~ ^"$LXPKG_ROOT" ]; then
            log "Removing binary symlink $bin_path/$pkg"
            rm -f "$bin_path/$pkg" || warn "Failed to remove $bin_path/$pkg"
            removed=$((removed + 1))
        fi
    done

    [ $removed -eq 0 ] && warn "No files explicitly removed for $pkg; cleaning up database entry"
    log "Removing $pkg from package database at $sys_db/$pkg"
    rm -rf "$sys_db/$pkg" || die "Failed to remove $sys_db/$pkg"
    log "Removed $pkg successfully"
}
