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

    for (GraphCell::size_type x = aCell.Location.x - 1; x <= aCell.Location.x + 1; ++x) {
        for (GraphCell::size_type y = aCell.Location.y - 1; y <= aCell.Location.y + 1; ++y) {
            GraphCell c(x, y);
            if (IsInside(c) && (x != aCell.Location.x || y != aCell.Location.y)
                && !mObstacles.contains(c)) {
                neighbours.emplace_back(c);
            }
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
