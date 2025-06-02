#!/bin/sh
# Check checksums (for logging only)
pkg_checksums() {
    pkg_find "$1"
    if [ -f "$repo_dir/checksums" ]; then
        local checksum_lines=$(wc -l < "$repo_dir/checksums" 2>/dev/null || echo 0)
        if [ "$checksum_lines" -gt 0 ]; then
            log "Found checksum file at $repo_dir/checksums with $checksum_lines entries for $1"
            return 0
        else
            warn "Checksum file $repo_dir/checksums is empty for $1"
            return 1
        fi
    else
        warn "No checksums file found at $repo_dir/checksums for $1"
        return 1
    fi
}

# Verify checksums
pkg_verify() {
    local pkg="$1"
    pkg_find "$pkg"
    if [ "$LXPKG_SKIP_CHECKSUMS" = "1" ]; then
        log "Checksum verification skipped for $pkg (LXPKG_SKIP_CHECKSUMS=1)"
        return 0
    fi
    if [ ! -f "$repo_dir/checksums" ]; then
        warn "No checksums file found at $repo_dir/checksums for $pkg"
        prompt "Checksum file missing for $pkg. Continue without verification?"
        return 0
    fi
    local checksum_lines=$(wc -l < "$repo_dir/checksums" 2>/dev/null || echo 0)
    if [ "$checksum_lines" -eq 0 ]; then
        warn "Checksum file $repo_dir/checksums is empty for $pkg"
        prompt "Empty checksum file for $pkg. Continue without verification?"
        return 0
    fi
    log "Verifying checksums for $pkg"
    local src_pkg_dir="$src_dir/$pkg" # Corrected variable name
    while read -r expected_checksum file; do
        [ -z "$file" ] && continue
        local full_path="$src_pkg_dir/$file" # Use corrected variable
        if [ ! -f "$full_path" ]; then
            warn "Source file $full_path not found for $pkg"
            prompt "Missing source file $file for $pkg. Continue?"
            continue
        fi
        local actual_checksum=$(sha256sum "$full_path" | awk '{print $1}')
        if [ "$actual_checksum" != "$expected_checksum" ]; then
            warn "Checksum mismatch for $file in $pkg: expected $expected_checksum, got $actual_checksum"
            prompt "Checksum verification failed for $file in $pkg. Continue?"
            continue
        fi
        log "Checksum verified for $file in $pkg"
    done < "$repo_dir/checksums"
    log "All checksums verified for $pkg"
    return 0
}
