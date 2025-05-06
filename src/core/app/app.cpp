#include "core/app/app.hpp"

#include <assimp/types.h>
#include <fmt/base.h>

#include "components/game.hpp"
#include "components/scene_object.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/event_handler.hpp"
#include "core/physics.hpp"

void Application::StartGameInstance(Registry& aRegistry, const GameInstanceID aGameID)
{
    fmt::println("spawning game instance");
    auto& physics = aRegistry.ctx().emplace<Physics>();
    aRegistry.ctx().emplace<ActionBuffer>();
    aRegistry.ctx().emplace<GameInstance>(aGameID, 0.0f, 0u);

    physics.Init(aRegistry);
    // TODO: leak ?
    physics.World()->setEventListener(new EventHandler(&aRegistry));
    SpawnMap(aRegistry, 20, 20);
    fmt::println("spawned game instance");
}

void Application::AdvanceSimulation(Registry& aRegistry, const float aDeltaTime)
{
    auto& actions  = aRegistry.ctx().get<ActionBuffer&>();
    auto& instance = aRegistry.ctx().get<GameInstance&>();

    instance.Accumulator += aDeltaTime;

    // While there is enough accumulated time to take
    // one or several physics steps
    while (instance.Accumulator >= kTimeStep) {
        // Decrease the accumulated time
        instance.Accumulator -= kTimeStep;

        for (const auto& system : mSystemsFT) {
            system(aRegistry, kTimeStep);
        }
        actions.Push();
        actions.Latest().GameID = instance.GameID;
        actions.Latest().Tick   = ++instance.Tick;
    }
}

void Application::SpawnMap(Registry& aRegistry, uint32_t aWidth, uint32_t aHeight)
{
    auto&        physics = aRegistry.ctx().get<Physics>();
    entt::entity first   = entt::null;

    // Create tiles
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

    // Create physics heightfield
    std::vector<float>         heightValues((aWidth + 1) * (aHeight + 1), 0.0f);
    std::vector<rp3d::Message> messages;
    rp3d::HeightField*         heightField = physics.Common().createHeightField(
        static_cast<int>(aWidth) + 1,
        static_cast<int>(aHeight) + 1,
        heightValues.data(),
        rp3d::HeightField::HeightDataType::HEIGHT_FLOAT_TYPE,
        messages);

    // Create physics body
    glm::vec3        translate = glm::vec3(aWidth + 1, 2.0f, aHeight + 1) / 4.0f - 0.5f;
    rp3d::Transform  transform(ToRP3D(translate), rp3d::Quaternion::identity());
    rp3d::RigidBody* body = physics.CreateRigidBody(
        first,
        aRegistry,
        RigidBodyParams{
            .Type           = rp3d::BodyType::STATIC,
            .Transform      = transform,
            .GravityEnabled = false});
    rp3d::HeightFieldShape* heightFieldShape = physics.Common().createHeightFieldShape(heightField);
    rp3d::Collider*         collider         = body->addCollider(heightFieldShape, transform);
    collider->setCollisionCategoryBits(Category::Terrain);
    collider->setCollideWithMaskBits(Category::Entities);
}
