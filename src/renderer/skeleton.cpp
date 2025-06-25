#include "renderer/skeleton.hpp"

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/spdlog.h>

#include <glm/gtx/string_cast.hpp>

void PrintBone(const Bone& aBone, const Skeleton& aSkeleton, std::size_t aIndent)
{
    if (aBone.Offset) {
        spdlog::info(
            "{}bone {} with offset {}",
            std::string(aIndent, ' '),
            aBone.Name,
            glm::to_string(*aBone.Offset));
    } else {
        spdlog::info("{}bone {} with no offset", std::string(aIndent, ' '), aBone.Name);
    }
    for (const auto bIdx : aBone.Children) {
        PrintBone(aSkeleton.Bones[bIdx], aSkeleton, aIndent + 2);
    }
}

void PrintSkeleton(const Skeleton& aSkeleton)
{
    spdlog::info("skeleton with {} bones", aSkeleton.Bones.size());
    if (!aSkeleton.Bones.empty()) {
        PrintBone(aSkeleton.Bones[0], aSkeleton, 0);
    }
}
