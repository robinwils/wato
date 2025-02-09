#pragma once
#include <renderer/material.hpp>
#include <renderer/primitive.hpp>

struct SceneObject {
    Primitive *primitive = nullptr;
    Material   material;
};