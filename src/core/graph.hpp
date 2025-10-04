#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/types.hpp"

struct GraphCell {
    using size_type         = uint16_t;
    using grid_preview_type = uint8_t;
    using vec_type          = glm::vec<2, size_type, glm::defaultp>;

    GraphCell(const size_type aX, const size_type aY) : Location(aX, aY) {}

    constexpr static size_type kCellsPerAxis = 3;

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
            Location.x / static_cast<float>(GraphCell::kCellsPerAxis),
            0.001f,
            Location.y / static_cast<float>(GraphCell::kCellsPerAxis));
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
    enum class Direction { Left, Right, Up, Down, UpRight, UpLeft, DownRight, DownLeft };
    Graph(const size_type aW, const size_type aH) : mWidth(aW), mHeight(aH) {}

    constexpr size_type Width() const { return mWidth; }
    constexpr size_type Height() const { return mHeight; }

    constexpr bool IsInside(GraphCell::size_type aX, GraphCell::size_type aY)
    {
        return aX < mWidth && aY < mHeight;
    }

    constexpr bool IsInside(const GraphCell& aCell)
    {
        return IsInside(aCell.Location.x, aCell.Location.y);
    }

    constexpr size_type Index(const GraphCell& aCell) const
    {
        return SafeU16(aCell.Location.y) * mWidth + aCell.Location.x;
    }

    constexpr std::optional<Graph::Direction> Direction(
        const GraphCell& aFrom,
        const GraphCell& aTo)
    {
        const int dx = int(aTo.Location.x) - int(aFrom.Location.x);
        const int dy = int(aTo.Location.y) - int(aFrom.Location.y);

        if (dx == -1 && dy == 0) return Direction::Left;
        if (dx == 1 && dy == 0) return Direction::Right;
        if (dx == 0 && dy == -1) return Direction::Up;
        if (dx == 0 && dy == 1) return Direction::Down;
        if (dx == 1 && dy == -1) return Direction::UpRight;
        if (dx == -1 && dy == -1) return Direction::UpLeft;
        if (dx == 1 && dy == 1) return Direction::DownRight;
        if (dx == -1 && dy == 1) return Direction::DownLeft;
        return std::nullopt;
    }

    grid_preview_type GridLayout() const;
    obstacles_type    Neighbours(const GraphCell& aCell);

    /**
     * @brief using a simple Flood Field/BFS algorithm, compute the Path from any cell to the
     * destination, result is stored internally.
     */
    void ComputePaths(const GraphCell& aDest);

    std::optional<GraphCell> GetNextCell(const GraphCell& aFrom)
    {
        if (!mPaths.contains(aFrom) || aFrom == mDest) {
            return std::nullopt;
        }
        return std::make_optional(mPaths.at(aFrom));
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
struct fmt::formatter<enum Graph::Direction> : fmt::formatter<std::string> {
    auto format(enum Graph::Direction aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        switch (aObj) {
            case Graph::Direction::Left:
                return fmt::format_to(aCtx.out(), "\u2190");
            case Graph::Direction::Right:
                return fmt::format_to(aCtx.out(), "\u2192");
            case Graph::Direction::Up:
                return fmt::format_to(aCtx.out(), "\u2191");
            case Graph::Direction::Down:
                return fmt::format_to(aCtx.out(), "\u2193");
            case Graph::Direction::UpRight:
                return fmt::format_to(aCtx.out(), "\u2197");
            case Graph::Direction::UpLeft:
                return fmt::format_to(aCtx.out(), "\u2196");
            case Graph::Direction::DownRight:
                return fmt::format_to(aCtx.out(), "\u2198");
            case Graph::Direction::DownLeft:
                return fmt::format_to(aCtx.out(), "\u2199");
            default:
                return fmt::format_to(aCtx.out(), "?");
        }
    }
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
        auto o = fmt::format_to(
            aCtx.out(),
            "{}x{} grid, {} paths:\n",
            aObj.mWidth,
            aObj.mHeight,
            aObj.mPaths.size());
        for (GraphCell::size_type y = 0; y < aObj.mHeight; ++y) {
            for (GraphCell::size_type x = 0; x < aObj.mWidth; ++x) {
                GraphCell c(x, y);
                if (aObj.mObstacles.contains(c)) {
                    o = fmt::format_to(o, "{}", 1);
                } else if (aObj.mDest == c) {
                    o = fmt::format_to(o, "*");
                } else if (aObj.mPaths.contains(c)) {
                    const GraphCell& to  = aObj.mPaths.at(c);
                    const auto&      dir = aObj.Direction(c, to);
                    if (dir) {
                        o = fmt::format_to(o, "{}", *dir);
                    } else {
                        o = fmt::format_to(o, "?");
                    }
                } else {
                    o = fmt::format_to(o, "{}", 0);
                }
            }
            o = fmt::format_to(o, "\n");
        }
        return fmt::format_to(o, "obstacles: {}", fmt::join(aObj.mObstacles, ", "));
    }
};
