#include "registry/game_registry.hpp"

#include <bgfx/bgfx.h>

#include <glm/ext/vector_float3.hpp>

#include "components/imgui.hpp"
#include "components/light_source.hpp"
#include "components/scene_object.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "core/physics.hpp"
#include "registry/registry.hpp"
#include "renderer/blinn_phong_material.hpp"
#include "renderer/plane_primitive.hpp"

void LoadResources(Registry& aRegistry)
{
    LoadShaders(aRegistry);
    SpawnLight(aRegistry);
    LoadTextures(aRegistry, 20, 20);
    LoadModels();
}

void LoadShaders(Registry& aRegistry)
{
    auto [bp, pLoaded] = WATO_PROGRAM_CACHE.load(
        "blinnphong"_hs,
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
    auto [c, cLoaded] = WATO_PROGRAM_CACHE.load(
        "simple"_hs,
        "vs_cubes",
        "fs_cubes",
        std::unordered_map<std::string, bgfx::UniformType::Enum>{});
}

void LoadTextures(Registry& aRegistry, uint32_t aWidth, uint32_t aHeight)
{
    auto [diff, dLoaded] =
        WATO_TEXTURE_CACHE.load("grass/diffuse"_hs, "assets/textures/TreeTop_COLOR.png");
    auto [sp, sLoaded] =
        WATO_TEXTURE_CACHE.load("grass/specular"_hs, "assets/textures/TreeTop_SPEC.png");

    auto shader   = WATO_PROGRAM_CACHE["blinnphong"_hs];
    auto diffuse  = WATO_TEXTURE_CACHE["grass/diffuse"_hs];
    auto specular = WATO_TEXTURE_CACHE["grass/specular"_hs];

    if (!bgfx::isValid(shader->Program())) {
        throw std::runtime_error("could not load blinnphong program, invalid handle");
    }
    if (!bgfx::isValid(diffuse)) {
        throw std::runtime_error("could not load grass/diffuse texture, invalid handle");
    }
    if (!bgfx::isValid(specular)) {
        throw std::runtime_error("could not load grass/specular texture, invalid handle");
    }

    WATO_MODEL_CACHE.load(
        "grass_tile"_hs,
        new PlanePrimitive(new BlinnPhongMaterial(shader, diffuse, specular)));
}

void SpawnLight(Registry& aRegistry)
{
    auto light = aRegistry.create();
    aRegistry.emplace<LightSource>(light, glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.5f));
    aRegistry.emplace<ImguiDrawable>(light, "Directional Light");
}

void LoadModels()
{
    WATO_MODEL_CACHE.load(
        "tower_model"_hs,
        "assets/models/tower.fbx",
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices
            | aiProcess_GlobalScale);
}
