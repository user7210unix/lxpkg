#!/bin/sh
# Utility functions
ok() { [ -e "$1" ] || return 1; }
equ() { [ "$1" = "$2" ]; }
contains() { case " $1 " in *" $2 "*) return 0 ;; esac; return 1; }
prompt() {
    printf '%b%s%b\n' "$c_green" "$*" "$c_reset"
    printf '%b%s%b' "$c_grey" "Continue? [y/N] " "$c_reset"
    read -r response
    case "$response" in
        [yY]*) return 0 ;;
        *) die "Operation aborted by user" ;;
    esac
}
