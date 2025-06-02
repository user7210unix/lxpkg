#!/bin/sh
# Search for packages
pkg_search() {
    local search_term="${1:-.}"
    log "Searching for packages matching '$search_term'..."
    set +f
    local found=0
    IFS=:
    for path in $LXPKG_PATH; do
        [ -d "$path" ] || continue
        for section in "$path"/*; do
            [ -d "$section" ] || continue
            for pkg_path in "$section"/*; do
                if [ -d "$pkg_path" ] && [ -f "$pkg_path/version" ]; then
                    local pkg_name=$(basename "$pkg_path")
                    if printf '%s' "$pkg_name" | grep -qi "$search_term"; then
                        found=1
                        read -r ver rel < "$pkg_path/version" 2>/dev/null || { ver="unknown"; rel="unknown"; }
                        local section_name=$(basename "$section")
                        printf '%b * %b%-20s %bSection:%b %s %bVersion:%b %s-%s\n' \
                            "$c_grey" "$c_cyan" "$pkg_name" "$c_grey" "$c_green" "$section_name" "$c_grey" "$c_green" "$ver" "$rel"
                    fi
                fi
            done
        done
    done
    unset IFS
    set -f
    [ "$found" -eq 0 ] && warn "No packages found matching '$search_term'"
    return 0
}
