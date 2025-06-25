#pragma once
#include <string>
#include <vector>

class ManifestGenerator {
public:
    static std::vector<std::string> generateManifest(const std::string& dir);
};