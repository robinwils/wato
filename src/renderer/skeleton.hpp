#pragma once

#include <spdlog/spdlog.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <optional>
#include <string>
#include <vector>

struct Bone {
    using index_type = std::size_t;

    std::string              Name;
    // this is to differentiate between nodes and bones
    glm::mat4                LocalTransform;
    std::optional<glm::mat4> Offset{std::nullopt};
    std::vector<index_type>  Children;
};

template <>
struct fmt::formatter<Bone> : fmt::formatter<std::string> {
    auto format(Bone aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "bone {} with {} children",
            aObj.Name,
            aObj.Children.size());
    }
};

struct Skeleton {
    using index_type = Bone::index_type;

    std::vector<Bone> Bones;
};

void PrintSkeleton(const Skeleton& aSkeleton);
