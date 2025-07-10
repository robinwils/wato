#include "registry/game_registry.hpp"

#include <bgfx/bgfx.h>

#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "components/imgui.hpp"
#include "components/light_source.hpp"
#include "registry/registry.hpp"
#include "renderer/blinn_phong_material.hpp"
#include "renderer/plane_primitive.hpp"
#include "resource/cache.hpp"

void LoadResources(Registry& aRegistry)
{
    LoadShaders(aRegistry);
    SpawnLight(aRegistry);
    LoadTextures(aRegistry, 20, 20);
    LoadModels(aRegistry);
}

void LoadShaders(Registry& aRegistry)
{
    aRegistry.ctx().get<ShaderCache>().load(
        "blinnphong"_hs,
        "vs_blinnphong",
        "fs_blinnphong",
        ShaderLoader::uniform_desc_map{
            {"s_diffuseTex",  {bgfx::UniformType::Sampler}},
            {"s_specularTex", {bgfx::UniformType::Sampler}},
            {"u_diffuse",     {bgfx::UniformType::Vec4}   },
            {"u_specular",    {bgfx::UniformType::Vec4}   },
            {"u_lightDir",    {bgfx::UniformType::Vec4}   },
            {"u_lightCol",    {bgfx::UniformType::Vec4}   }
    });
    aRegistry.ctx().get<ShaderCache>().load(
        "blinnphong_skinned"_hs,
        "vs_blinnphong_skinned",
        "fs_blinnphong",
        ShaderLoader::uniform_desc_map{
            {"s_diffuseTex",  {bgfx::UniformType::Sampler}  },
            {"s_specularTex", {bgfx::UniformType::Sampler}  },
            {"u_diffuse",     {bgfx::UniformType::Vec4}     },
            {"u_specular",    {bgfx::UniformType::Vec4}     },
            {"u_lightDir",    {bgfx::UniformType::Vec4}     },
            {"u_lightCol",    {bgfx::UniformType::Vec4}     },
            {"u_bones",       {bgfx::UniformType::Mat4, 128}}
    });
    aRegistry.ctx().get<ShaderCache>().load(
        "simple"_hs,
        "vs_simple",
        "fs_simple",
        ShaderLoader::uniform_desc_map{});
    aRegistry.ctx().get<ShaderCache>().load(
        "grid"_hs,
        "vs_grid",
        "fs_grid",
        ShaderLoader::uniform_desc_map{
            {"s_gridTex",  {bgfx::UniformType::Sampler}},
            {"u_gridInfo", {bgfx::UniformType::Vec4}   },
    });
}

void LoadTextures(Registry& aRegistry, uint32_t aWidth, uint32_t aHeight)
{
    auto [diff, dLoaded] = aRegistry.ctx().get<TextureCache>().load(
        "grass/diffuse"_hs,
        "assets/textures/TreeTop_COLOR.png");
    auto [sp, sLoaded] = aRegistry.ctx().get<TextureCache>().load(
        "grass/specular"_hs,
        "assets/textures/TreeTop_SPEC.png");

    auto shader   = aRegistry.ctx().get<ShaderCache>()["blinnphong"_hs];
    auto diffuse  = aRegistry.ctx().get<TextureCache>()["grass/diffuse"_hs];
    auto specular = aRegistry.ctx().get<TextureCache>()["grass/specular"_hs];

    if (!bgfx::isValid(shader->Program())) {
        throw std::runtime_error("could not load blinnphong program, invalid handle");
    }
    if (!bgfx::isValid(diffuse)) {
        throw std::runtime_error("could not load grass/diffuse texture, invalid handle");
    }
    if (!bgfx::isValid(specular)) {
        throw std::runtime_error("could not load grass/specular texture, invalid handle");
    }

    aRegistry.ctx().get<ModelCache>().load(
        "grass_tile"_hs,
        new PlanePrimitive(new BlinnPhongMaterial(shader, diffuse, specular)));
}

void SpawnLight(Registry& aRegistry)
{
    auto light = aRegistry.create();
    aRegistry.emplace<LightSource>(light, glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.5f));
    aRegistry.emplace<ImguiDrawable>(light, "Directional Light");
}

void LoadModels(Registry& aRegistry)
{
    aRegistry.ctx().get<ModelCache>().load(
        "tower_model"_hs,
        aRegistry,
        "assets/models/tower.fbx",
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices
            | aiProcess_GlobalScale);
    aRegistry.ctx().get<ModelCache>().load(
        "phoenix"_hs,
        aRegistry,
        "assets/models/phoenix.fbx",
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_GlobalScale | aiProcess_FlipUVs
            | aiProcess_ConvertToLeftHanded);
    aRegistry.ctx().get<ModelCache>().load(
        "mutant"_hs,
        aRegistry,
        "assets/models/mutant.fbx",
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_GlobalScale | aiProcess_FlipUVs
            | aiProcess_ConvertToLeftHanded);
}
