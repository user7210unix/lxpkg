#!/bin/sh
# Find package
pkg_find() {
    _pkg_find "$@" || return 1
}

_pkg_find() {
    local pkg="$1" find_type="${2:-}" test_type="${3:-}" search_path="${4:-$LXPKG_PATH}"
    IFS=:
    set +f
    local found=0
    for path in $search_path "${test_type:-$sys_db}"; do
        [ -d "$path" ] || continue
        for section in "$path"/*; do
            [ -d "$section" ] || continue
            for pkg_path in "$section"/*; do
                if [ -d "$pkg_path" ] && [ -f "$pkg_path/version" ] && [ "$(basename "$pkg_path")" = "$pkg" ]; then
                    found=1
                    repo_dir="$pkg_path"
                    repo_name="$pkg"
                    test "${test_type:--d}" "$pkg_path" && set -- "$@" "$pkg_path"
                fi
            done
        done
    done
    unset IFS
    set -f
    case $find_type-$# in
        *-4) return 1 ;;
        -*) : ;;
        *) shift 4; printf '%s\n' "$@"
    esac
    [ "$found" -eq 1 ] || return 1
}

# Find package version
pkg_find_version() {
    read -r ver_pre rel_pre < "$repo_dir/version" 2>/dev/null || { ver_pre=; rel_pre=; }
    repo_ver=$ver_pre
    repo_rel=${rel_pre:-0}
}

# Check if installed
pkg_is_installed() { [ -d "$sys_db/$1" ]; }
