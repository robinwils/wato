#include "core/graph.hpp"

Graph::grid_preview_type Graph::GridLayout() const
{
    Graph::grid_preview_type grid(Width * Height, 0);

    for (const GraphCell& obstacle : Obstacles) {
        grid[obstacle.Location.y * Width + obstacle.Location.y] = 1;
    }

    return grid;
}

Graph::obstacles_type Graph::Neighbours(const GraphCell& aCell) { return {}; }
