#include "lxpkg.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <package-name>\n", argv[0]);
        return 1;
    }
    build_package(argv[1]);
    return 0;
}

