#include "registry.hpp"

#include <bgfx/bgfx.h>

#include <glm/ext/vector_float3.hpp>

#include "components/camera.hpp"
#include "components/color.hpp"
#include "components/direction.hpp"
#include "components/imgui.hpp"
#include "components/scene_object.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "renderer/plane_primitive.hpp"

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
            emplace<Transform3D>(tile, glm::vec3(i, 0.0f, j), glm::vec3(0.0f), glm::vec3(1.0f));
            emplace<SceneObject>(tile, "grass_tile"_hs);
            emplace<Tile>(tile);
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
    MODEL_CACHE.load("tower_model"_hs,
        "assets/models/tower.fbx",
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices | aiProcess_GlobalScale);

    auto tower = create();
    emplace<Transform3D>(tower, glm::vec3(5.0f, 0.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.1f));
    emplace<SceneObject>(tower, "tower_model"_hs);
}

void Registry::spawnPlayerAndCamera()
{
    auto player = create();

    auto camera = create();
    emplace<Camera>(camera,
        // up, front, dir
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, -1.0f, -1.0f),
        // speed, fov, near, far
        2.5f,
        60.0f,
        0.1f,
        100.0f);
    // pos, rot, scale
    emplace<Transform3D>(camera, glm::vec3(0.0f, 2.0f, 1.5f), glm::vec3(1.0f), glm::vec3(1.0f));
    emplace<ImguiDrawable>(camera, "Camera");
}
