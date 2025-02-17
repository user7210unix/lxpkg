#include "lxpkg.h"

// Callback function for writing fetched data to a file
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

int fetch_toml(const char* pkgname, const char* dest_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return 1;
    }

    char url[256];
    snprintf(url, sizeof(url), "%s/%s/package.toml", REPO_URL, pkgname);

    FILE* file = fopen(dest_path, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", dest_path);
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    CURLcode res = curl_easy_perform(curl);

    fclose(file);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to fetch TOML: %s\n", curl_easy_strerror(res));
        return 1;
    }

    return 0;
}

int download_source(const char* url, const char* dest_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return 1;
    }

    FILE* file = fopen(dest_path, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", dest_path);
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    CURLcode res = curl_easy_perform(curl);

    fclose(file);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to download source: %s\n", curl_easy_strerror(res));
        return 1;
    }

    return 0;
}

