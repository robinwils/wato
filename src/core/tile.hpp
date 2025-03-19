#pragma once

#include <memory>
#include <renderer/plane_primitive.hpp>

#include "renderer/material.hpp"

struct Tile {
    enum Type {
        GRASS,
        COUNT,
    };

    std::shared_ptr<PlanePrimitive> Primitive;
    std::shared_ptr<Material>       Material;
    Type                            Type;
    bool                            Constructable;
};
