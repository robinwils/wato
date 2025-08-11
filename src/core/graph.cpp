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

    for (GraphCell::size_type i = aCell.Location.x - 1; i <= aCell.Location.x + 1; ++i) {
        for (GraphCell::size_type j = aCell.Location.y - 1; j <= aCell.Location.y + 1; ++j) {
            if (IsInside(i, j) && i != aCell.Location.x && j != aCell.Location.y) {
                neighbours.emplace_back(i, j);
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
