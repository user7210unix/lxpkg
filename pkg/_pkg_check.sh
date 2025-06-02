#!/bin/sh
# Check for pre-installed system packages
pkg_check_system() {
    local pkg="$1"
    local paths="/bin /usr/bin /sbin /usr/sbin /usr/local/bin /usr/local/sbin /lib /usr/lib /lib64 /usr/lib64 /opt"
    
    case "$pkg" in
        bash)
            for path in /bin/bash /usr/bin/bash; do
                [ -x "$path" ] && { log "$pkg detected at $path, skipping"; return 0; }
            done
            ;;
        glibc)
            for lib in /lib/libc.so* /usr/lib/libc.so* /lib64/libc.so* /usr/lib64/libc.so*; do
                [ -f "$lib" ] && { log "$pkg detected in $lib, skipping"; return 0; }
            done
            ;;
        xorg-server)
            [ -f "/usr/bin/Xorg" ] || [ -f "/usr/lib/xorg/Xorg" ] && { log "$pkg detected, skipping"; return 0; }
            ;;
        sed|grep|procps-ng)
            for path in /bin /usr/bin; do
                [ -x "$path/$pkg" ] && { log "$pkg detected at $path/$pkg, marking as system-installed"; mkdir -p "$sys_db/$pkg"; printf 'system system\n' > "$sys_db/$pkg/version"; return 0; }
            done
            ;;
        *)
            if [ -d "$sys_db/$pkg" ] && [ -f "$sys_db/$pkg/version" ]; then
                read -r ver rel < "$sys_db/$pkg/version" 2>/dev/null
                log "$pkg version $ver-$rel already installed, skipping"
                return 0
            fi
            for path in $paths; do
                if [ -f "$path/$pkg" ] || [ -f "$path/lib$pkg.so" ] || [ -d "$path/$pkg" ]; then
                    log "$pkg detected in $path, marking as system-installed"
                    mkdir -p "$sys_db/$pkg"
                    printf 'system system\n' > "$sys_db/$pkg/version"
                    return 0
                fi
            done
            ;;
    esac
    return 1
}
