#include "core/graph.hpp"

#include <queue>

Graph::grid_preview_type Graph::GridLayout() const
{
    Graph::grid_preview_type grid(mWidth * mHeight, 0);

    for (const GraphCell& obstacle : mObstacles) {
        grid[Index(obstacle)] = 255;
    }

    return grid;
}

Graph::obstacles_type Graph::Neighbours(const GraphCell& aCell)
{
    Graph::obstacles_type neighbours;

    // https://www.redblobgames.com/pathfinding/a-star/implementation.html#troubleshooting-ugly-path
    GraphCell candidates[8] = {
        // N, S, E, W
        GraphCell(aCell.Location.x, aCell.Location.y - 1),
        GraphCell(aCell.Location.x, aCell.Location.y + 1),
        GraphCell(aCell.Location.x + 1, aCell.Location.y),
        GraphCell(aCell.Location.x - 1, aCell.Location.y),
        // NE, NW, SE, SW
        GraphCell(aCell.Location.x + 1, aCell.Location.y - 1),
        GraphCell(aCell.Location.x - 1, aCell.Location.y - 1),
        GraphCell(aCell.Location.x + 1, aCell.Location.y + 1),
        GraphCell(aCell.Location.x - 1, aCell.Location.y + 1),
    };

    for (const GraphCell& c : candidates) {
        if (IsInside(c) && !mObstacles.contains(c)) {
            neighbours.emplace_back(c);
        }
    }

    return neighbours;
}

void Graph::ComputePaths(const GraphCell& aDest)
{
    mDest = aDest;
    mPaths.clear();
    std::queue<GraphCell> cells;

    cells.push(aDest);

    while (!cells.empty()) {
        GraphCell current = cells.front();
        cells.pop();
        for (const GraphCell& neighbour : Neighbours(current)) {
            if (!mPaths.contains(neighbour)) {
                cells.push(neighbour);
                mPaths.emplace(neighbour, current);
            }
        }
    }
}
