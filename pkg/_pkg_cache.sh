#!/bin/sh
# Check package cache
pkg_cache() {
    pkg_find_version "$1"
    local cache_file="$cache_dir/$1@$repo_ver-$repo_rel.tar.$LXPKG_COMPRESS"
    if [ -f "$cache_file" ]; then
        log "Found cached package $1@$repo_ver-$repo_rel"
        mkdir -p "$pkg_dir/$1"
        tar -C "$pkg_dir/$1" -xvf "$cache_file" >/dev/null
        return 0
    fi
    return 1
}

# Clean cache
pkg_clean_cache() {
    log "Cleaning package cache..."
    set +f
    for cache in "$cache_dir"/*.tar.*; do
        [ -f "$cache" ] || continue
        rm -f "$cache"
        log "Removed $cache"
    done
    set -f
    log "Cache cleaned."
}
