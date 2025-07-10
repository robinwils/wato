#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <unordered_set>
#include <vector>

struct GraphCell {
    static const uint32_t      kCellsPerAxis = 3;
    static constexpr GraphCell ToGrid(float aX, float aY)
    {
        return GraphCell{
            .Location = glm::uvec2(
                static_cast<uint32_t>(aX * kCellsPerAxis),
                static_cast<uint32_t>(aY * kCellsPerAxis)),
        };
    }
    static constexpr GraphCell ToGrid(glm::vec3 aPoint) { return ToGrid(aPoint.x, aPoint.z); }

    bool operator==(const GraphCell&) const = default;

    glm::uvec2 Location;
};

constexpr inline glm::vec3 GridToWorld(const uint32_t aX, const uint32_t aY)
{
    return glm::vec3(aX / GraphCell::kCellsPerAxis, 0.0f, aY / GraphCell::kCellsPerAxis);
}

constexpr inline glm::vec3 GridToWorld(const glm::uvec2& aGridCoords)
{
    return GridToWorld(aGridCoords.x, aGridCoords.y);
}

constexpr inline glm::vec3 GridToWorld(const GraphCell& aCell)
{
    return GridToWorld(aCell.Location);
}

template <>
struct std::hash<GraphCell> {
    std::size_t operator()(const GraphCell& aCell) const noexcept
    {
        std::size_t hX = std::hash<uint32_t>{}(aCell.Location.x);
        std::size_t hY = std::hash<uint32_t>{}(aCell.Location.y);
        return hX ^ (hY << 1);
    }
};

struct Graph {
    Graph(const uint32_t aW, const uint32_t aH) : Width(aW), Height(aH) {}
    uint32_t Width;
    uint32_t Height;

    std::unordered_set<GraphCell> Obstacles;

    std::vector<GraphCell> Neighbours(const GraphCell& aCell);
};
