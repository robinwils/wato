#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <unordered_set>
#include <vector>

struct GraphCell {
    using size_type         = uint16_t;
    using grid_preview_type = uint8_t;
    using vec_type          = glm::vec<2, size_type, glm::defaultp>;

    GraphCell(const size_type aX, const size_type aY) : Location(aX, aY) {}

    constexpr static float kCellsPerAxis = 3.0f;

    static GraphCell FromWorldPoint(float aX, float aZ)
    {
        return GraphCell{
            static_cast<size_type>(aX * kCellsPerAxis),
            static_cast<size_type>(aZ * kCellsPerAxis),
        };
    }
    static GraphCell FromWorldPoint(glm::vec3 aPoint) { return FromWorldPoint(aPoint.x, aPoint.z); }

    constexpr glm::vec3 ToWorld() const
    {
        return glm::vec3(
            Location.x / GraphCell::kCellsPerAxis,
            0.001f,
            Location.y / GraphCell::kCellsPerAxis);
    }

    bool operator==(const GraphCell&) const = default;

    vec_type Location;
};

template <>
struct std::hash<GraphCell> {
    std::size_t operator()(const GraphCell& aCell) const noexcept
    {
        std::size_t hX = std::hash<GraphCell::size_type>{}(aCell.Location.x);
        std::size_t hY = std::hash<GraphCell::size_type>{}(aCell.Location.y);
        return hX ^ (hY << 1);
    }
};

class Graph
{
    using size_type         = GraphCell::size_type;
    using grid_preview_type = std::vector<GraphCell::grid_preview_type>;
    using obstacles_type    = std::vector<GraphCell>;
    using paths_type        = std::unordered_map<GraphCell, GraphCell>;

   public:
    Graph(const size_type aW, const size_type aH) : mWidth(aW), mHeight(aH) {}

    constexpr size_type Width() const { return mWidth; }
    constexpr size_type Height() const { return mHeight; }

    constexpr bool IsInside(GraphCell::size_type aX, GraphCell::size_type aY)
    {
        return aX >= 0 && aY >= 0 && aX < mWidth && aY < mHeight;
    }

    constexpr size_type Index(const GraphCell& aCell) const
    {
        return aCell.Location.y * mWidth + aCell.Location.x;
    }

    grid_preview_type GridLayout() const;
    obstacles_type    Neighbours(const GraphCell& aCell);

    /**
     * @brief using a simple Flood Field/BFS algorithm, compute the Path from any cell to the
     * destination, result is stored internally.
     */
    void ComputePaths(const GraphCell& aDest);

    constexpr std::optional<GraphCell> GetNextCell(const GraphCell& aFrom)
    {
        return mPaths.contains(aFrom) ? std::make_optional(mPaths.at(aFrom)) : std::nullopt;
    }

    void AddObstacle(const GraphCell& aCell) { mObstacles.emplace(aCell); }
    void RemoveObstacle(const GraphCell& aCell) { mObstacles.erase(aCell); }

   private:
    size_type mWidth;
    size_type mHeight;

    std::unordered_set<GraphCell> mObstacles;
    paths_type                    mPaths;
    std::optional<GraphCell>      mDest;

    friend class fmt::formatter<Graph>;
};

template <>
struct fmt::formatter<GraphCell> : fmt::formatter<std::string> {
    auto format(GraphCell aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "[{}, {}]", aObj.Location.x, aObj.Location.y);
    }
};

template <>
struct fmt::formatter<Graph> : fmt::formatter<std::string> {
    auto format(Graph aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        auto o = fmt::format_to(aCtx.out(), "{}x{} grid:\n", aObj.mWidth, aObj.mHeight);
        for (GraphCell::size_type i = 0; i < aObj.mHeight; ++i) {
            for (GraphCell::size_type j = 0; j < aObj.mWidth; ++j) {
                o = fmt::format_to(o, "{}", aObj.mObstacles.contains(GraphCell(j, i)) ? 1 : 0);
            }
            o = fmt::format_to(o, "\n");
        }
        return fmt::format_to(o, "obstacles: {}", fmt::join(aObj.mObstacles, ", "));
    }
};
