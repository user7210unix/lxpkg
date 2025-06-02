#!/bin/sh
# Get sources (for displaying)
pkg_sources() {
    pkg_find "$1"
    [ -f "$repo_dir/sources" ] && cat "$repo_dir/sources" || { warn "No sources file found for $1"; return 1; }
}

# Download and extract sources (for building)
pkg_source() {
    pkg_find "$1" || die "Package '$1' not found"
    mkdir -p "$src_dir/$1" || die "Failed to create source directory $src_dir/$1"
    cd "$src_dir/$1" || die "Failed to change to source directory $src_dir/$1"
    if [ ! -f "$repo_dir/sources" ]; then
        warn "No sources file found at $repo_dir/sources for $1"
        return 1
    fi
    local i=1 has_sources=0
    while read -r url file; do
        [ -z "$url" ] && continue
        has_sources=1
        file=${file:-$(basename "$url")}
        log "Processing source #$i: $url"
        if [ ! -f "$file" ]; then
            log "Downloading $1: $url to $file..."
            if ! curl -L "$url" -o "$file" 2>/dev/null; then
                warn "Failed to download $url for $1"
                return 1
            fi
            log "Downloaded $1: $file successfully"
        fi
        case "$file" in # Added quotes for robustness
            *.tar.gz|*.tgz)
                log "Extracting $file to $src_dir/$1"
                if ! tar -xzf "$file" -C "$src_dir/$1" --strip-components=1 2>/dev/null; then
                    warn "Failed to extract $file for $1"
                    return 1
                fi
                ;;
            *.tar.bz2|*.tbz2)
                log "Extracting $file to $src_dir/$1"
                if ! tar -xjf "$file" -C "$src_dir/$1" --strip-components=1 2>/dev/null; then
                    warn "Failed to extract $file for $1"
                    return 1
                fi
                ;;
            *.tar.xz|*.txz)
                log "Extracting $file to $src_dir/$1"
                if ! tar -xJf "$file" -C "$src_dir/$1" --strip-components=1 2>/dev/null; then
                    warn "Failed to extract $file for $1"
                    return 1
                fi
                ;;
            *.zip)
                log "Extracting $file to $src_dir/$1"
                if ! unzip -q "$file" -d "$src_dir/$1" 2>/dev/null; then
                    warn "Failed to extract $file for $1"
                    return 1
                fi
                # Move files from subdirectory if created
                local extracted_dir=$(find "$src_dir/$1" -maxdepth 1 -type d -not -path "$src_dir/$1" | head -n 1)
                if [ -n "$extracted_dir" ] && [ "$extracted_dir" != "$src_dir/$1" ]; then
                    log "Moving files from $extracted_dir to $src_dir/$1"
                    mv "$extracted_dir"/* "$extracted_dir"/.[!.]* "$src_dir/$1" 2>/dev/null || warn "Failed to move extracted files"
                    rmdir "$extracted_dir" 2>/dev/null || warn "Failed to remove extracted directory"
                fi
                ;;
            *)
                warn "Unknown archive type for $file, skipping extraction"
                continue
                ;;
        esac
        i=$((i + 1))
    done < "$repo_dir/sources"
    if [ "$has_sources" -eq 0 ]; then
        warn "No valid source URLs found in $repo_dir/sources for $1"
        return 1
    fi
    if ! find "$src_dir/$1" -maxdepth 1 -type f | grep -q .; then
        warn "Source directory $src_dir/$1 is empty or contains no files for $1"
        ls -la "$src_dir/$1" >&2
        return 1
    fi
    log "Source files prepared for $1 in $src_dir/$1"
    return 0
}
