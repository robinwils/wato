#pragma once

#include "core/graph.hpp"

struct Path {
    GraphCell                LastFrom;
    std::optional<GraphCell> NextCell;
};
