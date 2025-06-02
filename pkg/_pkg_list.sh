#!/bin/sh
# List installed packages
pkg_list() {
    set +f
    local found_pkgs=0
    for pkg_dir_entry in "$sys_db/"*; do
        if [ -d "$pkg_dir_entry" ] && [ -f "$pkg_dir_entry/version" ]; then
            local pkg_name=$(basename "$pkg_dir_entry")
            read -r ver rel < "$pkg_dir_entry/version" 2>/dev/null || { ver="unknown"; rel="unknown"; }
            printf '%b%s %b%s-%s%b\n' "$c_cyan" "$pkg_name" "$c_green" "$ver" "$rel" "$c_reset"
            found_pkgs=$((found_pkgs + 1))
        fi
    done
    set -f
    if [ "$found_pkgs" -eq 0 ]; then
        warn "No packages installed."
    fi
}
