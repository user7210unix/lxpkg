#include "util.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <openssl/evp.h>
#include <iomanip>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace util {
    std::mutex consoleMutex;
    std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("lxpkg");

    bool fileExists(const std::string& path) {
        struct stat buffer;
        return stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode);
    }

    bool dirExists(const std::string& path) {
        struct stat buffer;
        return stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode);
    }

    std::string captureCommandOutput(const std::string& cmd) {
        std::string result;
        FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
        if (!pipe) {
            logger->error("Failed to open pipe for command: {}", cmd);
            return "";
        }
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::lock_guard<std::mutex> lock(consoleMutex);
            result += buffer;
        }
        pclose(pipe);
        return result;
    }

    std::string computeSha256(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            logger->error("Failed to open file for SHA256: {}", filePath);
            return "";
        }
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        if (!mdctx) {
            logger->error("Failed to create EVP_MD_CTX");
            return "";
        }
        if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
            logger->error("Failed to initialize SHA256 context");
            EVP_MD_CTX_free(mdctx);
            return "";
        }
        char buffer[8192];
        while (file.good()) {
            file.read(buffer, sizeof(buffer));
            if (EVP_DigestUpdate(mdctx, buffer, file.gcount()) != 1) {
                logger->error("Failed to update SHA256 digest");
                EVP_MD_CTX_free(mdctx);
                return "";
            }
        }
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hashLen;
        if (EVP_DigestFinal_ex(mdctx, hash, &hashLen) != 1) {
            logger->error("Failed to finalize SHA256 digest");
            EVP_MD_CTX_free(mdctx);
            return "";
        }
        EVP_MD_CTX_free(mdctx);
        std::stringstream ss;
        for (unsigned int i = 0; i < hashLen; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
}