#pragma once

#include <spdlog/spdlog.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct Bone {
    using index_type = std::size_t;
    std::string              Name;
    // this is to differentiate between nodes and bones
    std::optional<glm::mat4> Offset{std::nullopt};
    std::vector<index_type>  Children;
};

struct Skeleton {
    using index_type     = Bone::index_type;
    using bone_index_map = std::unordered_map<std::string, index_type>;

    std::vector<Bone> Bones;
};

void PrintSkeleton(const Skeleton& aSkeleton);
