#include "registry/game_registry.hpp"

#include <bgfx/bgfx.h>

#include <glm/ext/vector_float3.hpp>

#include "components/camera.hpp"
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
    SpawnMap(aRegistry, 20, 20);
    LoadModels();
    SpawnPlayerAndCamera(aRegistry);
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

void SpawnMap(Registry& aRegistry, uint32_t aWidth, uint32_t aHeight)
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
    entt::entity first = entt::null;
    for (uint32_t i = 0; i < aWidth; ++i) {
        for (uint32_t j = 0; j < aHeight; ++j) {
            auto tile = aRegistry.create();
            if (first == entt::null) {
                first = tile;
            }
            aRegistry.emplace<Transform3D>(
                tile,
                glm::vec3(i, 0.0f, j),
                glm::vec3(0.0f),
                glm::vec3(1.0f));
            aRegistry.emplace<SceneObject>(tile, "grass_tile"_hs);
            aRegistry.emplace<Tile>(tile);
        }
    }

    auto&              phy     = aRegistry.ctx().get<Physics&>();
    int                columns = aWidth + 1, rows = aHeight + 1;
    std::vector<float> heightValues(columns * rows, 0.0f);

    std::vector<rp3d::Message> messages;
    rp3d::HeightField*         heightField = phy.Common().createHeightField(
        columns,
        rows,
        heightValues.data(),
        rp3d::HeightField::HeightDataType::HEIGHT_FLOAT_TYPE,
        messages);

    // Display the messages (info, warning and errors)
    if (messages.size() > 0) {
        for (const rp3d::Message& message : messages) {
            std::string messageType;

            switch (message.type) {
                case rp3d::Message::Type::Information:
                    messageType = "info";
                    break;
                case rp3d::Message::Type::Warning:
                    messageType = "warning";
                    break;
                case rp3d::Message::Type::Error:
                    messageType = "error";
                    break;
            }

            fmt::println("Message ({}): {}", messageType, message.text);
        }
    }

    // Make sure there was no errors during the height field creation
    assert(heightField != nullptr);

    // by default rp3d heightfield is centered around origin, we need to translate it in our world
    // pos
    glm::vec3        translate = glm::vec3(columns, 2.0f, rows) / 4.0f - 0.5f;
    rp3d::Transform  transform(ToRP3D(translate), rp3d::Quaternion::identity());
    rp3d::RigidBody* body = phy.CreateRigidBody(
        first,
        aRegistry,
        RigidBodyParams{
            .Type           = rp3d::BodyType::STATIC,
            .Transform      = transform,
            .GravityEnabled = false});
    rp3d::HeightFieldShape* heightFieldShape = phy.Common().createHeightFieldShape(heightField);
    rp3d::Collider*         collider         = body->addCollider(heightFieldShape, transform);
    collider->setCollisionCategoryBits(Category::Terrain);
    collider->setCollideWithMaskBits(Category::Entities);
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

void SpawnPlayerAndCamera(Registry& aRegistry)
{
    auto player = aRegistry.create();

    auto camera = aRegistry.create();
    aRegistry.emplace<Camera>(camera,
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
    aRegistry.emplace<Transform3D>(camera,
        glm::vec3(0.0f, 2.0f, 1.5f),
        glm::vec3(1.0f),
        glm::vec3(1.0f));
    aRegistry.emplace<ImguiDrawable>(camera, "Camera");
}
