#pragma once

#include <memory>
#include <renderer/plane_primitive.hpp>

struct Tile {
    enum Type {
        Grass,
        Count,
    };

    std::shared_ptr<PlanePrimitive> primitive;
    std::shared_ptr<Material>       material;
    Type                            type;
    bool                            constructable;
};