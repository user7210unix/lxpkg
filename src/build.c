#include "lxpkg.h"
#include <libgen.h>
#include <unistd.h>

void build_package(const char* pkgname) {
    char toml_path[PATH_MAX];
    snprintf(toml_path, sizeof(toml_path), "/tmp/%s.toml", pkgname);

    if (fetch_toml(pkgname, toml_path) != 0) {
        fprintf(stderr, "Failed to fetch TOML for %s\n", pkgname);
        return;
    }

    char* version = get_toml_value(toml_path, "package.version");
    char* sources = get_toml_value(toml_path, "source.urls");

    printf("Building %s-%s\n", pkgname, version);

    char source_path[PATH_MAX];
    snprintf(source_path, sizeof(source_path), "/tmp/%s-%s.tar.gz", pkgname, version);
    if (download_source(sources, source_path)) {
        fprintf(stderr, "Failed to download source for %s\n", pkgname);
        return;
    }

    system("mkdir -p build_dir");

    char command[PATH_MAX];
    snprintf(command, sizeof(command), "tar xvf %s -C build_dir", source_path);
    system(command);

    chdir("build_dir");
    system("./configure --prefix=/usr");
    system("make");
    system("make DESTDIR=../pkg_root install");
    chdir("..");

    char lpkg_name[256];
    snprintf(lpkg_name, sizeof(lpkg_name), "%s/%s-%s.lpkg", PKG_CACHE, pkgname, version);
    snprintf(command, sizeof(command), "tar czvf %s -C pkg_root .", lpkg_name);
    system(command);

    free(version);
    free(sources);
}

