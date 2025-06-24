#ifndef LXPKG_UTIL_HPP
#define LXPKG_UTIL_HPP
#include <string>
namespace util {
    bool fileExists(const std::string& path);
    bool dirExists(const std::string& path);
}
#endif
