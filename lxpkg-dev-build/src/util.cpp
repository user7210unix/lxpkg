#include "util.hpp"
#include <sys/stat.h>
namespace util {
    bool fileExists(const std::string& path) {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
    }
    bool dirExists(const std::string& path) {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
    }
}
