#include "registry.hpp"

#include <components/color.hpp>
#include <components/direction.hpp>
#include <components/position.hpp>
#include <components/rotation.hpp>
#include <components/scale.hpp>
#include <components/scene_object.hpp>
#include <core/cache.hpp>
#include <core/sys.hpp>
#include <glm/ext/vector_float3.hpp>
#include <renderer/plane_primitive.hpp>

#include "bgfx/bgfx.h"
#include "glm/trigonometric.hpp"

void Registry::spawnMap(uint32_t _w, uint32_t _h)
{
    auto [bp, pLoaded]   = PROGRAM_CACHE.load("blinnphong"_hs, "vs_blinnphong", "fs_blinnphong");
    auto [diff, dLoaded] = TEXTURE_CACHE.load("grass/diffuse"_hs, "assets/textures/TreeTop_COLOR.png");
    auto [sp, sLoaded]   = TEXTURE_CACHE.load("grass/specular"_hs, "assets/textures/TreeTop_SPEC.png");

    auto program  = PROGRAM_CACHE["blinnphong"_hs];
    auto diffuse  = TEXTURE_CACHE["grass/diffuse"_hs];
    auto specular = TEXTURE_CACHE["grass/specular"_hs];

    if (!bgfx::isValid(program)) {
        throw std::runtime_error("could not load blinnphong program, invalid handle");
    }
    if (!bgfx::isValid(diffuse)) {
        throw std::runtime_error("could not load grass/diffuse texture, invalid handle");
    }
    if (!bgfx::isValid(specular)) {
        throw std::runtime_error("could not load grass/specular texture, invalid handle");
    }

    MODEL_CACHE.load("grass_tile"_hs, new PlanePrimitive(Material(program, diffuse, specular)));

    for (uint32_t i = 0; i < _w; ++i) {
        for (uint32_t j = 0; j < _h; ++j) {
            auto tile = create();
            emplace<Position>(tile, glm::vec3(i, 0, j));
            emplace<Rotation>(tile, glm::vec3(0, 0, 0));
            emplace<Scale>(tile, glm::vec3(1.0f));
            emplace<SceneObject>(tile, "grass_tile"_hs);
        }
    }
}

void Registry::spawnLight()
{
    auto light = create();
    emplace<Direction>(light, glm::vec3(-1.0f, -1.0f, 0.0f));
    emplace<Color>(light, glm::vec3(0.5f));
}

void Registry::spawnModel()
{
    MODEL_CACHE.load("tower_model"_hs, "assets/models/tower.fbx");

    auto tower = create();
    emplace<Position>(tower, glm::vec3(0, 0, 0));
    emplace<Rotation>(tower, glm::vec3(0));
    emplace<Scale>(tower, glm::vec3(0.1f));
    emplace<SceneObject>(tower, "tower_model"_hs);
}
