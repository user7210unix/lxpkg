#include "lxpkg.h"
#include <toml.h>
#include <stdlib.h>
#include <string.h>

char* get_toml_value(const char* toml_path, const char* key) {

    FILE* file = fopen(toml_path, "r");
    if (!file) {
        fprintf(stderr, "Failed to open TOML file: %s\n", toml_path);
        return NULL;
    }


    char errbuf[200];
    toml_table_t* table = toml_parse_file(file, errbuf, sizeof(errbuf));
    fclose(file);

    if (!table) {
        fprintf(stderr, "Failed to parse TOML file: %s\n", errbuf);
        return NULL;
    }

    toml_raw_t raw = toml_raw_in(table, key);
    if (raw.ok) {
        char* result = strdup(raw.u.s);
        toml_free(table);
        return result;
    }

    // Free the TOML table
    toml_free(table);
    return NULL;
}

