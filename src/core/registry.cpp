#include "registry.hpp"

#include <bgfx/bgfx.h>

#include <glm/ext/vector_float3.hpp>

#include "components/camera.hpp"
#include "components/imgui.hpp"
#include "components/light_source.hpp"
#include "components/scene_object.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "renderer/plane_primitive.hpp"

void Registry::spawnMap(uint32_t _w, uint32_t _h)
{
    auto [bp, pLoaded] = PROGRAM_CACHE.load("blinnphong"_hs,
        "vs_blinnphong",
        "fs_blinnphong",
        std::unordered_map<std::string, bgfx::UniformType::Enum>{
            {"s_diffuseTex",  bgfx::UniformType::Sampler},
            {"s_specularTex", bgfx::UniformType::Sampler},
            {"u_diffuse",     bgfx::UniformType::Vec4   },
            {"u_specular",    bgfx::UniformType::Vec4   },
            {"u_lightDir",    bgfx::UniformType::Vec4   },
            {"u_lightCol",    bgfx::UniformType::Vec4   }
    });

    auto [diff, dLoaded] =
        TEXTURE_CACHE.load("grass/diffuse"_hs, "assets/textures/TreeTop_COLOR.png");
    auto [sp, sLoaded] =
        TEXTURE_CACHE.load("grass/specular"_hs, "assets/textures/TreeTop_SPEC.png");

    auto shader   = PROGRAM_CACHE["blinnphong"_hs];
    auto diffuse  = TEXTURE_CACHE["grass/diffuse"_hs];
    auto specular = TEXTURE_CACHE["grass/specular"_hs];

    if (!bgfx::isValid(shader->program())) {
        throw std::runtime_error("could not load blinnphong program, invalid handle");
    }
    if (!bgfx::isValid(diffuse)) {
        throw std::runtime_error("could not load grass/diffuse texture, invalid handle");
    }
    if (!bgfx::isValid(specular)) {
        throw std::runtime_error("could not load grass/specular texture, invalid handle");
    }

    MODEL_CACHE.load("grass_tile"_hs, new PlanePrimitive(Material(shader, diffuse, specular)));

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
    emplace<LightSource>(light, glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.5f));
    emplace<ImguiDrawable>(light, "Directional Light");
}

void Registry::loadModels()
{
    MODEL_CACHE.load("tower_model"_hs,
        "assets/models/tower.fbx",
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices
            | aiProcess_GlobalScale);
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
