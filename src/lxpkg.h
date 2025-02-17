#ifndef LXPKG_H
#define LXPKG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <toml.h>

#define MAX_LINE 1024
#define PKG_DB_DIR "/var/lib/lxpkg/db"
#define PKG_CACHE "/var/cache/lxpkg/pkgs"
#define REPO_URL "https://raw.githubusercontent.com/LearnixOS/lxos-repo/main"

// Fetch TOML file from GitHub
int fetch_toml(const char* pkgname, const char* dest_path);

// Download source archive
int download_source(const char* url, const char* dest_path);

// Parse TOML and get value
char* get_toml_value(const char* toml_path, const char* key);

#endif

