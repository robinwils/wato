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
    LoadTextures(aRegistry);
    LoadModels(aRegistry);
}

void LoadShaders(Registry& aRegistry)
{
    LoadResource(
        aRegistry.ctx().get<ShaderCache>(),
        "blinnphong",
        "vs_blinnphong",
        "fs_blinnphong",
        ShaderLoader::uniform_desc_map{
            // clang-format off
            {"s_diffuseTex",  {bgfx::UniformType::Sampler}},
            {"s_specularTex", {bgfx::UniformType::Sampler}},
            {"u_diffuse",     {bgfx::UniformType::Vec4}   },
            {"u_specular",    {bgfx::UniformType::Vec4}   },
            {"u_lightDir",    {bgfx::UniformType::Vec4}   },
            {"u_lightCol",    {bgfx::UniformType::Vec4}   },
            // clang-format on
        });
    LoadResource(
        aRegistry.ctx().get<ShaderCache>(),
        "blinnphong_skinned",
        "vs_blinnphong_skinned",
        "fs_blinnphong",
        ShaderLoader::uniform_desc_map{
            // clang-format off
            {"s_diffuseTex",  {bgfx::UniformType::Sampler}  },
            {"s_specularTex", {bgfx::UniformType::Sampler}  },
            {"u_diffuse",     {bgfx::UniformType::Vec4}     },
            {"u_specular",    {bgfx::UniformType::Vec4}     },
            {"u_lightDir",    {bgfx::UniformType::Vec4}     },
            {"u_lightCol",    {bgfx::UniformType::Vec4}     },
            {"u_bones",       {bgfx::UniformType::Mat4, 128}},
            // clang-format on
        });
    LoadResource(
        aRegistry.ctx().get<ShaderCache>(),
        "blinnphong_instanced",
        "vs_blinnphong_instanced",
        "fs_blinnphong",
        ShaderLoader::uniform_desc_map{
            // clang-format off
            {"s_diffuseTex",  {bgfx::UniformType::Sampler}},
            {"s_specularTex", {bgfx::UniformType::Sampler}},
            {"u_diffuse",     {bgfx::UniformType::Vec4}   },
            {"u_specular",    {bgfx::UniformType::Vec4}   },
            {"u_lightDir",    {bgfx::UniformType::Vec4}   },
            {"u_lightCol",    {bgfx::UniformType::Vec4}   },
            // clang-format on
        });
    LoadResource(
        aRegistry.ctx().get<ShaderCache>(),
        "simple",
        "vs_simple",
        "fs_simple",
        ShaderLoader::uniform_desc_map{});
    LoadResource(
        aRegistry.ctx().get<ShaderCache>(),
        "grid",
        "vs_grid",
        "fs_grid",
        ShaderLoader::uniform_desc_map{
            // clang-format off
            {"s_gridTex",  {bgfx::UniformType::Sampler}},
            {"u_gridInfo", {bgfx::UniformType::Vec4}   },
            // clang-format on
        });
}

void LoadTextures(Registry& aRegistry)
{
    const auto& diffuse = LoadResource(
        aRegistry.ctx().get<TextureCache>(),
        "grass/diffuse",
        "assets/textures/TreeTop_COLOR.png");
    const auto& specular = LoadResource(
        aRegistry.ctx().get<TextureCache>(),
        "grass/specular",
        "assets/textures/TreeTop_SPEC.png");

    auto shader = aRegistry.ctx().get<ShaderCache>()["blinnphong_instanced"_hs];

    LoadResource(
        aRegistry.ctx().get<ModelCache>(),
        "grass_tile",
        std::make_unique<PlanePrimitive>(
            std::make_unique<BlinnPhongMaterial>(shader, diffuse, specular)));
}

void SpawnLight(Registry& aRegistry)
{
    auto light = aRegistry.create();
    aRegistry.emplace<LightSource>(light, glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.5f));
    aRegistry.emplace<ImguiDrawable>(light, "Directional Light");
}

void LoadModels(Registry& aRegistry)
{
    LoadResource(
        aRegistry.ctx().get<ModelCache>(),
        "tower_model",
        aRegistry,
        "assets/models/tower.fbx",
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices
            | aiProcess_GlobalScale);
    LoadResource(
        aRegistry.ctx().get<ModelCache>(),
        "phoenix",
        aRegistry,
        "assets/models/phoenix.fbx",
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_GlobalScale | aiProcess_FlipUVs
            | aiProcess_ConvertToLeftHanded);
    LoadResource(
        aRegistry.ctx().get<ModelCache>(),
        "mutant",
        aRegistry,
        "assets/models/mutant.fbx",
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_GlobalScale | aiProcess_FlipUVs
            | aiProcess_ConvertToLeftHanded);
}
