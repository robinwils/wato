#include "core/app/app.hpp"

#include <cpr/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <glaze/glaze.hpp>

#include "components/game.hpp"
#include "components/health.hpp"
#include "components/player.hpp"
#include "components/rigid_body.hpp"
#include "components/scene_object.hpp"
#include "components/spawner.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/physics/physics.hpp"
#include "core/physics/physics_event_listener.hpp"
#include "core/state.hpp"
#include "input/action.hpp"

using namespace entt::literals;

void Application::Init()
{
    // CPR's thread pool has a bug where idle_thread_num (atomic size_t)
    // underflows to SIZE_MAX when threads are cleaned up after the default
    // 250ms idle timeout, permanently preventing new thread creation.
    // Use a longer idle timeout to avoid triggering the underflow.
    cpr::async::startup(1, std::thread::hardware_concurrency(), std::chrono::minutes(5));

    auto err = glz::read_file_json(mGameplayDef, "data/gameplay.json", std::string{});
    if (err) {
        mLogger->error("failed to load gameplay definitions: {}", glz::format_error(err));
        return;
    }
    mLogger->info("loaded gameplay definitions");
}

void Application::StartGameInstance(Registry& aRegistry, const GameInstanceID aGameID)
{
    WATO_INFO(aRegistry, "spawning game instance");
    auto& physics = aRegistry.ctx().emplace<Physics>(mLogger);

    aRegistry.ctx().emplace<GameStateBuffer>();
    aRegistry.ctx().emplace<GameInstance>(aGameID, 0.0f, 0u);
    aRegistry.ctx().emplace<ColliderEntityMap>();
    aRegistry.ctx().emplace<FixedSystemExecutor>();

    physics.Init();

    auto& l = aRegistry.ctx().emplace<PhysicsEventListener>(mLogger);
    GetSingletonComponent<Physics>(aRegistry).World()->setEventListener(&l);

    SetupObservers(aRegistry);
}

void Application::StopGameInstance(Registry& aRegistry)
{
    aRegistry.ctx().erase<Physics>();
    aRegistry.ctx().erase<ActionContextStack>();
    aRegistry.ctx().erase<GameStateBuffer>();
    aRegistry.ctx().erase<GameInstance>();
}

void Application::SetupObservers(Registry& aRegistry)
{
    auto& observers = aRegistry.ctx().emplace<Observers>();

    observers.push_back("tower_built_observer");
    auto& tbo = aRegistry.storage<entt::reactive>("tower_built_observer"_hs);
    tbo.on_construct<Tower>();

    observers.push_back("rigid_bodies_observer");
    aRegistry.storage<entt::reactive>("rigid_bodies_observer"_hs)
        .on_construct<RigidBody>()
        .on_update<RigidBody>();

    observers.push_back("rigid_bodies_destroy_observer");
    aRegistry.storage<entt::reactive>("rigid_bodies_destroy_observer"_hs).on_destroy<RigidBody>();

    observers.push_back("placement_mode_observer");
    auto& pmo = aRegistry.storage<entt::reactive>("placement_mode_observer"_hs);
    pmo.on_update<Transform3D>();
    mLogger->info("observers created");
}

void Application::SpawnTerrain(
    Registry&           aRegistry,
    const entt::entity& aPlayer,
    const glm::uvec2&   aSize,
    const glm::vec2&    aOffset)
{
    const Player& p     = aRegistry.get<Player>(aPlayer);
    entt::entity  first = entt::null;

    for (uint32_t i = 0; i < aSize.x; ++i) {
        for (uint32_t j = 0; j < aSize.y; ++j) {
            auto tile = aRegistry.create();
            if (first == entt::null) {
                first = tile;
            }
            aRegistry.emplace<Transform3D>(
                tile,
                glm::vec3(float(i) + aOffset.x, 0.0f, float(j) + aOffset.y));
            aRegistry.emplace<SceneObject>(tile, "grass_tile"_hs);
            aRegistry.emplace<Tile>(tile);
        }
    }

    auto spawner = aRegistry.create();
    aRegistry.emplace<Transform3D>(spawner, glm::vec3(float(aOffset.x), 0.0f, float(aOffset.y)));
    aRegistry.emplace<Spawner>(spawner);
    aRegistry.emplace<Owner>(spawner, p.ID, p.Slot);

    aRegistry.emplace<RigidBody>(
        first,
        RigidBody{
            .Params =
                RigidBodyParams{
                    .Type           = rp3d::BodyType::STATIC,
                    .Velocity       = 0.0f,
                    .Direction      = glm::vec3(0.0f),
                    .GravityEnabled = false,
                },
        });
    aRegistry.emplace<Collider>(
        first,
        Collider{
            .Params =
                ColliderParams{
                    .CollisionCategoryBits = Category::Terrain,
                    .CollideWithMaskBits   = Category::Projectiles,
                    .IsTrigger             = false,
                    .Offset =
                        Transform3D{
                            .Position = glm::vec3(aSize.x + 1, 1.004f, aSize.y + 1) / 2.0f - 0.5f,
                        },
                    .ShapeParams =
                        HeightFieldShapeParams{
                            .Data    = std::vector<float>((aSize.x + 1) * (aSize.y + 1), 0.0f),
                            .Rows    = SafeI32(aSize.y + 1),
                            .Columns = SafeI32(aSize.x + 1),
                        },
                },
        });
}
