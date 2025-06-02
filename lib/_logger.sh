#!/bin/sh
# Logging functions
log() {
    [ "${LXPKG_VERBOSE:-0}" -eq 1 ] || return 0
    printf '%b%s %b%s%b\n' "$c_grey" "==>" "$c_green" "$*" "$c_reset"
}
warn() { printf '%b%s %b%s%b\n' "$c_yellow" "WARNING:" "$c_reset" "$*" "$c_reset" >&2; }
die() { printf '%b%s %b%s%b\n' "$c_red" "ERROR:" "$c_reset" "$*" "$c_reset" >&2; exit 1; }
