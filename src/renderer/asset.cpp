#include "asset.hpp"

#include <filesystem>

#include "core/sys/log.hpp"

std::string FindAsset(const char* aFileName)
{
    TRACE("finding asset {}", aFileName);
    if (std::filesystem::exists(aFileName)) {
        return aFileName;
    }

    for (const char* aDir : kSearchPaths) {
        std::filesystem::path assetPath(std::filesystem::path(aDir) / aFileName);
        if (std::filesystem::exists(assetPath)) {
            return assetPath.string();
        }
    }

    return "";
}
