#include "core/app/app.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "components/game.hpp"
#include "components/player.hpp"
#include "components/rigid_body.hpp"
#include "components/scene_object.hpp"
#include "components/spawner.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/graph.hpp"
#include "core/physics/event_handler.hpp"
#include "core/physics/physics.hpp"
#include "core/state.hpp"
#include "input/action.hpp"

using namespace entt::literals;

void Application::Init() {}

void Application::StartGameInstance(
    Registry&            aRegistry,
    const GameInstanceID aGameID,
    const bool           aIsServer)
{
    WATO_INFO(aRegistry, "spawning game instance");
    auto& physics = aRegistry.ctx().emplace<Physics>(mLogger);
    auto& stack   = aRegistry.ctx().emplace<ActionContextStack>();

    aRegistry.ctx().emplace<GameStateBuffer>();
    aRegistry.ctx().emplace<GameInstance>(aGameID, 0.0f, 0u);

    physics.Init();

    stack.push_back(ActionContext{
        .State    = aIsServer ? ActionContext::State::Server : ActionContext::State::Default,
        .Bindings = ActionBindings::Defaults(),
        .Payload  = NormalPayload{}});

    SetupObservers(aRegistry);
    SpawnMap(aRegistry, 20, 20);
    OnGameInstanceCreated();
}

void Application::StopGameInstance(Registry& aRegistry)
{
    aRegistry.ctx().erase<Physics>();
    aRegistry.ctx().erase<ActionContextStack>();
    aRegistry.ctx().erase<GameStateBuffer>();
    aRegistry.ctx().erase<GameInstance>();
}

void Application::AdvanceSimulation(Registry& aRegistry, const float aDeltaTime)
{
    auto& state    = aRegistry.ctx().get<GameStateBuffer&>();
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
        state.Push();
        state.Latest().Tick = ++instance.Tick;
        ClearAllObservers(aRegistry);
    }
}

void Application::SpawnMap(Registry& aRegistry, uint32_t aWidth, uint32_t aHeight)
{
    entt::entity first = entt::null;

    auto& graph = aRegistry.ctx().emplace<Graph>(
        aWidth * GraphCell::kCellsPerAxis,
        aHeight * GraphCell::kCellsPerAxis);
    // Create tiles
    for (uint32_t i = 0; i < aWidth; ++i) {
        for (uint32_t j = 0; j < aHeight; ++j) {
            auto tile = aRegistry.create();
            if (first == entt::null) {
                first = tile;
            }
            aRegistry.emplace<Transform3D>(tile, glm::vec3(i, 0.0f, j));
            aRegistry.emplace<SceneObject>(tile, "grass_tile"_hs);
            aRegistry.emplace<Tile>(tile);
        }
    }

    // create spawn and base
    auto spawner = aRegistry.create();
    aRegistry.emplace<Transform3D>(spawner, glm::vec3(0.0f));
    aRegistry.emplace<Spawner>(spawner);

    auto  base          = aRegistry.create();
    auto& baseTransform = aRegistry.emplace<Transform3D>(base, glm::vec3(2.0f, 0.004f, 2.0f));
    aRegistry.emplace<Base>(base);
    aRegistry.emplace<RigidBody>(
        base,
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
        base,
        Collider{
            .Params =
                ColliderParams{
                    .CollisionCategoryBits = Category::Entities,
                    .CollideWithMaskBits   = Category::Terrain | Category::Entities,
                    .IsTrigger             = true,
                    .ShapeParams =
                        BoxShapeParams{
                            .HalfExtents = GraphCell(1, 1).ToWorld() * 0.5f,
                        },
                },
        });

    graph.ComputePaths(GraphCell::FromWorldPoint(baseTransform.Position));
    WATO_DBG(aRegistry, "{}", graph);

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
                    .CollideWithMaskBits   = Category::Entities,
                    .IsTrigger             = false,
                    .Offset =
                        Transform3D{
                            .Position = glm::vec3(aWidth + 1, 1.004f, aHeight + 1) / 2.0f - 0.5f,
                        },
                    .ShapeParams =
                        HeightFieldShapeParams{
                            .Data    = std::vector<float>((aWidth + 1) * (aHeight + 1), 0.0f),
                            .Rows    = SafeI32(aHeight + 1),
                            .Columns = SafeI32(aWidth + 1),
                        },
                },
        });
}

void Application::ClearAllObservers(Registry& aRegistry)
{
    for (const entt::hashed_string& hash : mObserverNames) {
        auto* storage = aRegistry.storage(hash);

        if (storage == nullptr) {
            throw std::runtime_error(fmt::format("{} storage not initiated", hash.data()));
        }

        storage->clear();
    }
}

void Application::SetupObservers(Registry& aRegistry)
{
    mObserverNames.push_back("tower_built_observer");
    auto& tbo = aRegistry.storage<entt::reactive>("tower_built_observer"_hs);
    tbo.on_construct<Tower>();

    mObserverNames.push_back("rigid_bodies_observer");
    auto& rbo = aRegistry.storage<entt::reactive>("rigid_bodies_observer"_hs);
    rbo.on_construct<RigidBody>();
    rbo.on_update<RigidBody>();

    mObserverNames.push_back("placement_mode_observer");
    auto& pmo = aRegistry.storage<entt::reactive>("placement_mode_observer"_hs);
    pmo.on_update<Transform3D>();
}
