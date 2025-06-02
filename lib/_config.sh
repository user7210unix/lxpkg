#!/bin/sh
# Create default /etc/lxpkg.conf
create_default_conf() {
    local conf_file="/etc/lxpkg.conf"
    if [ ! -f "$conf_file" ]; then
        log "Creating default configuration file at $conf_file"
        printf '# lxpkg configuration file\n' > "$conf_file"
        printf 'LXPKG_PATH=/usr/src/lxpkg/repo\n' >> "$conf_file"
        printf 'LXPKG_COMPRESS=xz\n' >> "$conf_file"
        printf 'LXPKG_SKIP_CHECKSUMS=0\n' >> "$conf_file"
        printf 'LXPKG_STRIP=1\n' >> "$conf_file"
        printf 'LXPKG_COLOR=1\n' >> "$conf_file"
        printf 'LXPKG_VERBOSE=0\n' >> "$conf_file"
    fi
}
