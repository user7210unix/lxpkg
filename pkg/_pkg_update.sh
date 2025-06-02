#!/bin/sh
# Update repositories
pkg_update() {
    log "Updating package repositories..."
    IFS=:
    for repo in $LXPKG_PATH; do
        [ -d "$repo" ] || { warn "Repository path $repo does not exist"; continue; }
        local repo_url="https://github.com/LearnixOS/repo/archive/refs/heads/main.tar.gz"
        local repo_name_github="LearnixOS-repo" # Renamed to avoid conflict with repo_name global
        log "Updating repository $repo_name_github from $repo_url"
        local tmp_dir=$(mktemp -d)
        if curl -L "$repo_url" -o "$tmp_dir/repo.tar.gz" 2>/dev/null; then
            mkdir -p "$tmp_dir/repo"
            tar -C "$tmp_dir/repo" -xzf "$tmp_dir/repo.tar.gz" --strip-components=1 || { warn "Failed to extract $repo_url"; rm -rf "$tmp_dir"; continue; }
            rm -rf "$repo" || warn "Failed to remove old repository $repo"
            mv "$tmp_dir/repo" "$repo" || warn "Failed to update repository $repo"
            log "Repository $repo_name_github updated successfully"
        else
            warn "Failed to download $repo_url for $repo_name_github"
        fi
        rm -rf "$tmp_dir"
    done
    unset IFS
    log "Repository update completed"
}
