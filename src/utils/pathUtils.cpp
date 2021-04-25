#include <string>
#include <filesystem>

bool validatePath(std::string basePath, std::string relativePath) {
    if (relativePath.empty() || relativePath[0] != '/') {
        return false;
    }
    std::string combined = basePath + relativePath;
    std::string canonicalBase = std::filesystem::weakly_canonical(basePath);
    std::string combinedCanonical = std::filesystem::weakly_canonical(combined);
    return combinedCanonical.substr(0, canonicalBase.size()) == canonicalBase;
}
