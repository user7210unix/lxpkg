#!/bin/sh
# Global variables (will be initialized in main after config is loaded)
# LXPKG_ROOT=""
# sys_db=""
# cache_dir=""
# src_dir=""
# bld_dir=""
# pkg_dir=""
# pkg_db=""
# sys_ch=""
# cho_db=""

# Variable to hold the found repository directory and package name
repo_dir=""
repo_name=""
repo_ver=""
repo_rel=""

# Dependency tracking
_seen_deps=""
_dep_stack=""
deps="" # Global variable for pkg_depends output
