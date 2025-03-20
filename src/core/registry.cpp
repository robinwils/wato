#include "registry.hpp"

#include <bgfx/bgfx.h>

#include <entt/signal/dispatcher.hpp>
#include <glm/ext/vector_float3.hpp>

#include "components/camera.hpp"
#include "components/imgui.hpp"
#include "components/light_source.hpp"
#include "components/physics.hpp"
#include "components/scene_object.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "core/event_handler.hpp"
#include "input/input.hpp"
#include "renderer/blinn_phong_material.hpp"
#include "renderer/physics.hpp"
#include "renderer/plane_primitive.hpp"

void Registry::Init(WatoWindow *aWin, EventHandler *aPhyEventHandler)
{
    ctx().emplace<entt::dispatcher>();
    ctx().emplace<Input &>(aWin->GetInput());
    auto &phy = ctx().emplace<Physics>();
    phy.World = phy.Common.createPhysicsWorld();

    phy.World->setEventListener(aPhyEventHandler);

    // Create the default logger
    rp3d::DefaultLogger *logger = phy.Common.createDefaultLogger();

    // Output the logs into the standard output
    logger->addStreamDestination(std::cout,
        static_cast<uint>(rp3d::Logger::Level::Error),
        rp3d::DefaultLogger::Format::Text);

    ctx().emplace<PhysicsParams>(false, false, true, logger);

#if WATO_DEBUG
    phy.World->setIsDebugRenderingEnabled(true);
#endif

    LoadShaders();
    SpawnLight();
    SpawnMap(20, 20);
    LoadModels();
    SpawnPlayerAndCamera();
}

void Registry::LoadShaders()
{
    auto [bp, pLoaded] = WATO_PROGRAM_CACHE.load("blinnphong"_hs,
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
    auto [c, cLoaded]  = WATO_PROGRAM_CACHE.load("simple"_hs,
        "vs_cubes",
        "fs_cubes",
        std::unordered_map<std::string, bgfx::UniformType::Enum>{});
}

void Registry::SpawnMap(uint32_t aWidth, uint32_t aHeight)
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

    WATO_MODEL_CACHE.load("grass_tile"_hs,
        new PlanePrimitive(new BlinnPhongMaterial(shader, diffuse, specular)));

    for (uint32_t i = 0; i < aWidth; ++i) {
        for (uint32_t j = 0; j < aHeight; ++j) {
            auto tile = create();
            emplace<Transform3D>(tile, glm::vec3(i, 0.0f, j), glm::vec3(0.0f), glm::vec3(1.0f));
            emplace<SceneObject>(tile, "grass_tile"_hs);
            emplace<Tile>(tile);
        }
    }
}

void Registry::SpawnLight()
{
    auto light = create();
    emplace<LightSource>(light, glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.5f));
    emplace<ImguiDrawable>(light, "Directional Light");
}

void Registry::LoadModels()
{
    WATO_MODEL_CACHE.load("tower_model"_hs,
        "assets/models/tower.fbx",
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices
            | aiProcess_GlobalScale);
}

void Registry::SpawnPlayerAndCamera()
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
