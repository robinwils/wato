#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <unordered_set>
#include <vector>

struct GraphCell {
    using size_type         = uint16_t;
    using grid_preview_type = uint8_t;
    using vec_type          = glm::vec<2, size_type, glm::defaultp>;

    static const size_type     kCellsPerAxis = 3;
    GraphCell(const size_type aX, const size_type aY) : Location(aX, aY) {}


    static GraphCell ToGrid(float aX, float aY)
    {
        return GraphCell{
            static_cast<size_type>(aX * kCellsPerAxis),
            static_cast<size_type>(aY * kCellsPerAxis),
        };
    }
    static GraphCell ToGrid(glm::vec3 aPoint) { return ToGrid(aPoint.x, aPoint.z); }

    bool operator==(const GraphCell&) const = default;

    vec_type Location;
};

constexpr inline glm::vec3 GridToWorld(const GraphCell::size_type aX, const GraphCell::size_type aY)
{
    return glm::vec3(aX / GraphCell::kCellsPerAxis, 0.0f, aY / GraphCell::kCellsPerAxis);
}

constexpr inline glm::vec3 GridToWorld(const GraphCell::vec_type& aGridCoords)
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
        std::size_t hX = std::hash<GraphCell::size_type>{}(aCell.Location.x);
        std::size_t hY = std::hash<GraphCell::size_type>{}(aCell.Location.y);
        return hX ^ (hY << 1);
    }
};

struct Graph {
    using size_type         = GraphCell::size_type;
    using grid_preview_type = std::vector<GraphCell::grid_preview_type>;
    using obstacles_type    = std::vector<GraphCell>;

    Graph(const size_type aW, const size_type aH) : Width(aW), Height(aH) {}
    size_type Width;
    size_type Height;

    std::unordered_set<GraphCell> Obstacles;

    grid_preview_type GridLayout() const;
    obstacles_type    Neighbours(const GraphCell& aCell);

    constexpr size_type Index(const GraphCell& aCell) const
    {
        return aCell.Location.y * Width + aCell.Location.x;
    }
};
