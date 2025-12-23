#pragma once

#include <memory>
#include <renderer/plane_primitive.hpp>

#include "renderer/material.hpp"

struct Tile {
    enum Type {
        GRASS,
        COUNT,
    };

    Type Type;
    bool Constructable;
};
