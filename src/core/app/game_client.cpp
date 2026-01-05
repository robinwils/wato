#include "core/app/game_client.hpp"

#include <bx/bx.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <entt/core/fwd.hpp>
#include <memory>
#include <span>

#include "components/game.hpp"
#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/model_rotation_offset.hpp"
#include "components/net.hpp"
#include "components/projectile.hpp"
#include "components/tower_attack.hpp"
#include "core/net/enet_client.hpp"
#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"
#include "core/types.hpp"
#include "core/window.hpp"
#include "registry/game_registry.hpp"
#include "registry/registry.hpp"
#include "renderer/grid_preview_material.hpp"
#include "renderer/primitive.hpp"
#include "renderer/renderer.hpp"
#include "systems/action.hpp"
#include "systems/animation.hpp"
#include "systems/input.hpp"
#include "systems/physics.hpp"
#include "systems/render.hpp"
#include "systems/rigid_bodies_update.hpp"
#include "systems/sync.hpp"
#include "systems/system.hpp"
#include "systems/tower_built.hpp"

using namespace std::literals::chrono_literals;
using namespace entt::literals;

void GameClient::Init()
{
    Application::Init();

    spdlog::set_default_logger(mLogger);

    auto& window    = mRegistry.ctx().get<WatoWindow>();
    auto& renderer  = mRegistry.ctx().get<BgfxRenderer&>();
    auto& netClient = mRegistry.ctx().get<ENetClient>();

    window.Init();
    renderer.Init(window);
    netClient.Init();

    LoadResources(mRegistry);

    // Register frame-time systems (variable delta)
    // in reversed order, entt::scheduler executes processes starting from the end
#if WATO_DEBUG
    mFrameExecutor.Register<PhysicsDebugSystem>();
#endif
    mFrameExecutor.Register<RenderSystem>();
    mFrameExecutor.Register<RenderImguiSystem>();
    mFrameExecutor.Register<SimulationSystem>();
    mFrameExecutor.Register<CameraSystem>();
    mFrameExecutor.Register<AnimationSystem>();
    mFrameExecutor.Register<RealTimeActionSystem>();
    mFrameExecutor.Register<InputSystem>();
}

int GameClient::Run(tf::Executor& aExecutor)
{
    auto& window    = mRegistry.ctx().get<WatoWindow&>();
    auto& renderer  = mRegistry.ctx().get<BgfxRenderer&>();
    auto& netClient = mRegistry.ctx().get<ENetClient&>();
    auto  prevTime  = clock_type::now();

    std::thread networkThreadHandle([this]() { networkThread(); });
    networkThreadHandle.detach();

    if (!netClient.Connect()) {
        throw std::runtime_error("No available peers for initiating an ENet connection.");
    }
    WATO_DBG(mRegistry, "connected to server");

    while (!window.ShouldClose()) {
        window.PollEvents();

        if (window.Resize()) {
            renderer.Resize(window);
        }

        Input&                       input     = window.GetInput();
        auto                         now       = clock_type::now();
        std::chrono::duration<float> frameTime = (now - prevTime);

        prevTime = now;

        consumeNetworkResponses();

        if (mRegistry.ctx().contains<GameInstance>()) {
            mFrameExecutor.Update(frameTime.count(), &mRegistry);
        }

        renderer.Render();
        input.PrevKeyboardState   = input.KeyboardState;
        input.PrevMouseState      = input.MouseState;
        input.MouseState.Scroll.y = 0;
    }

    StopGameInstance(mRegistry);
    netClient.Disconnect();
    mDiscTimerStart.emplace(clock_type::now());

    return 0;
}

void GameClient::networkThread()
{
    auto& netClient = mRegistry.ctx().get<ENetClient&>();
    while (netClient.Running()) {
        if (mDiscTimerStart) {
            if (clock_type::now() - *mDiscTimerStart > 3s) {
                netClient.ForceDisconnect();
            }
        }

        netClient.ConsumeNetworkRequests([&](NetworkRequest* aEvent) {
            // write header
            BitOutputArchive archive;

            aEvent->Archive(archive);

            netClient.Send(archive.Bytes());
        });
        netClient.Poll();
    }
}

void GameClient::spawnPlayerAndCamera()
{
    auto camera = mRegistry.create();
    mRegistry.emplace<Camera>(
        camera,
        // up, front, dir
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, -1.0f, -1.0f),
        // speed, fov, near, far
        2.5f,
        60.0f,
        0.1f,
        100.0f);
    // pos, orientation, scale
    mRegistry.emplace<Transform3D>(
        camera,
        glm::vec3(3.0f, 2.0f, 3.0f),
        glm::identity<glm::quat>(),
        glm::vec3(1.0f));
    mRegistry.emplace<ImguiDrawable>(camera, "Camera");

    auto player = mRegistry.create();
    // TODO: ID should be something coming from outside (menu, DB, etc...)
    mRegistry.emplace<Player>(player, 0u);
    mRegistry.emplace<Name>(player, "stion");
}

void GameClient::prepareGridPreview()
{
    const auto& graph = mRegistry.ctx().get<Graph>();

    GraphCell::size_type numVertsX = graph.Width() + 1;
    GraphCell::size_type numVertsY = graph.Height() + 1;

    std::vector<PositionVertex> vertices;
    std::vector<uint16_t>       indices;

    for (GraphCell::size_type i = 0; i < numVertsY; ++i) {
        for (GraphCell::size_type j = 0; j < numVertsX; ++j) {
            GraphCell cell(j, i);
            vertices.emplace_back(cell.ToWorld());
            if (i != 0) {
                indices.push_back(SafeU16(i) * numVertsX + j);
                indices.push_back((SafeU16(i) - 1) * numVertsX + j);
            }
            if (j != 0) {
                indices.push_back(SafeU16(i) * numVertsX + j);
                indices.push_back(SafeU16(i) * numVertsX + j - 1);
            }
        }
    }

    const auto& texture = LoadResource(
        mRegistry.ctx().get<TextureCache>(),
        "grid_tex",
        graph.Width(),
        graph.Height(),
        false,
        1,
        bgfx::TextureFormat::R8,
        0,
        nullptr);

    const entt::resource<Shader>& shader = mRegistry.ctx().get<ShaderCache>()["grid"_hs];
    auto                          mat    = std::make_unique<GridPreviewMaterial>(
        shader,
        glm::vec4(graph.Width(), graph.Height(), GraphCell::kCellsPerAxis, 0),
        texture);
    auto primitive = std::make_unique<Primitive<PositionVertex>>(vertices, indices, std::move(mat));

    // Put texture in context variables because I am not sure entt:resource_cache can be updated
    // easily
    LoadResource(mRegistry.ctx().get<ModelCache>(), "grid", std::move(primitive));
    auto& renderer = mRegistry.ctx().get<BgfxRenderer&>();
    renderer.UpdateTexture2D(
        texture,
        0,
        0,
        0,
        0,
        graph.Width(),
        graph.Height(),
        std::span<const uint8_t>(graph.GridLayout().data(), graph.Width() * graph.Height()));
}

void GameClient::OnGameInstanceCreated(Registry& aRegistry)
{
    auto& fixedExec = aRegistry.ctx().get<FixedSystemExecutor>();

    spawnPlayerAndCamera();
    prepareGridPreview();

    fixedExec.Register<NetworkSyncSystem<ENetClient>>();
    fixedExec.Register<PhysicsSystem>();
    fixedExec.Register<RigidBodiesUpdateSystem>();
    fixedExec.Register<TowerBuiltSystem>();
    fixedExec.Register<DeterministicActionSystem>();
}

void GameClient::consumeNetworkResponses()
{
    auto& netClient = mRegistry.ctx().get<ENetClient>();
    netClient.ConsumeNetworkResponses([&](NetworkResponse* aEvent) {
        std::visit(
            VariantVisitor{
                [&](const ConnectedResponse&) {
                    WATO_INFO(mRegistry, "got connected response");

                    // TODO: should be triggered by player input (menu, matchmaking,...)
                    netClient.EnqueueRequest(new NetworkRequest{
                        .Type     = PacketType::NewGame,
                        .PlayerID = 0,
                        .Tick     = 0,
                        .Payload  = NewGameRequest{.PlayerAID = 0},
                    });
                },
                [&](const NewGameResponse& aResp) {
                    StartGameInstance(mRegistry, aResp.GameID, false);
                    WATO_INFO(mRegistry, "game {} created", aResp.GameID);
                },
                [&](SyncPayload& aResp) {
                    if (aResp.State.Snapshot.empty()) {
                        return;
                    }

                    BitInputArchive       inAr(aResp.State.Snapshot);
                    Registry              tmp;
                    entt::snapshot_loader loader{tmp};

                    WATO_TRACE(
                        mRegistry,
                        "loading state snapshot {} of size {}",
                        aResp.State.Tick,
                        aResp.State.Snapshot.size());
                    loader.get<entt::entity>(inAr)
                        .get<Transform3D>(inAr)
                        .get<RigidBody>(inAr)
                        .get<Collider>(inAr);
                },
                [&](const RigidBodyUpdateResponse& aUpdate) {
                    auto& syncMap = mRegistry.ctx().get<EntitySyncMap>();

                    switch (aUpdate.Event) {
                        case RigidBodyEvent::Create: {
                            // Handle entity creation
                            std::visit(
                                VariantVisitor{
                                    [&](const ProjectileInitData& aProjectileInit) {
                                        WATO_INFO(mRegistry, "got projectile init data");
                                        // Map source tower entity
                                        auto sourceTowerIt =
                                            syncMap.find(aProjectileInit.SourceTower);
                                        if (sourceTowerIt == syncMap.end()) {
                                            WATO_WARN(
                                                mRegistry,
                                                "source tower {} not found in sync map",
                                                aProjectileInit.SourceTower);
                                            return;
                                        }

                                        entt::entity clientTower = sourceTowerIt->second;
                                        auto*        towerTransform =
                                            mRegistry.try_get<Transform3D>(clientTower);
                                        if (!towerTransform) {
                                            WATO_WARN(
                                                mRegistry,
                                                "source tower {} has no transform",
                                                clientTower);
                                            return;
                                        }

                                        // Create projectile
                                        auto projectile = mRegistry.create();

                                        glm::vec3 position =
                                            towerTransform->Position + glm::vec3(0.0f, 0.5f, 0.0f);

                                        mRegistry.emplace<Transform3D>(
                                            projectile,
                                            position,
                                            glm::identity<glm::quat>(),
                                            glm::vec3(0.05f));

                                        mRegistry.emplace<ModelRotationOffset>(
                                            projectile,
                                            glm::angleAxis(
                                                glm::radians(180.0f),
                                                glm::vec3(0, 1, 0)));

                                        // Map target if it exists
                                        entt::entity clientTarget = entt::null;
                                        auto targetIt = syncMap.find(aProjectileInit.Target);
                                        if (targetIt != syncMap.end()) {
                                            clientTarget = targetIt->second;
                                        }

                                        mRegistry.emplace<Projectile>(
                                            projectile,
                                            aProjectileInit.Damage,
                                            aProjectileInit.Speed,
                                            clientTarget);

                                        mRegistry.emplace<RigidBody>(
                                            projectile,
                                            RigidBody{.Params = aUpdate.Params});

                                        mRegistry.emplace<Collider>(
                                            projectile,
                                            Collider{
                                                .Params =
                                                    ColliderParams{
                                                        .CollisionCategoryBits =
                                                            Category::Projectiles,
                                                        .CollideWithMaskBits = Category::Entities,
                                                        .IsTrigger           = true,
                                                        .Offset              = Transform3D{},
                                                        .ShapeParams =
                                                            CapsuleShapeParams{
                                                                .Radius = 0.05f,
                                                                .Height = 0.1f,
                                                            },
                                                    },
                                            });

                                        mRegistry.emplace<SceneObject>(projectile, "arrow"_hs);

                                        syncMap.insert_or_assign(aUpdate.Entity, projectile);

                                        WATO_INFO(
                                            mRegistry,
                                            "created projectile {} from server entity {}",
                                            projectile,
                                            aUpdate.Entity);
                                    },
                                    [&](const TowerInitData& aTowerInit) {
                                        // Create tower on client
                                        auto  tower = mRegistry.create();
                                        auto& phy   = mRegistry.ctx().get<Physics>();

                                        auto& transform = mRegistry.emplace<Transform3D>(
                                            tower,
                                            aTowerInit.Position,
                                            glm::identity<glm::quat>(),
                                            glm::vec3(0.1f));

                                        // Create physics body and collider
                                        Collider collider{
                                            .Params =
                                                ColliderParams{
                                                    .CollisionCategoryBits = Category::Entities,
                                                    .CollideWithMaskBits =
                                                        Category::Terrain | Category::Entities,
                                                    .IsTrigger = false,
                                                    .Offset    = Transform3D{},
                                                    .ShapeParams =
                                                        BoxShapeParams{
                                                            .HalfExtents =
                                                                glm::vec3(0.35f, 0.65f, 0.35f),
                                                        },
                                                },
                                        };
                                        rp3d::RigidBody* body =
                                            phy.CreateRigidBody(aUpdate.Params, transform);
                                        rp3d::Collider* c = phy.AddCollider(body, collider.Params);

                                        mRegistry.emplace<Tower>(tower, aTowerInit.Type);
                                        mRegistry.emplace<RigidBody>(tower, aUpdate.Params, body);
                                        mRegistry.emplace<Collider>(tower, collider.Params, c);
                                        mRegistry.emplace<Health>(tower, 100.0f);
                                        mRegistry.emplace<SceneObject>(tower, "tower_model"_hs);
                                        mRegistry.emplace<TowerAttack>(
                                            tower,
                                            TowerAttack{
                                                .Range    = 30.0f,
                                                .FireRate = 1.0f,
                                            });

                                        syncMap.insert_or_assign(aUpdate.Entity, tower);

                                        WATO_INFO(
                                            mRegistry,
                                            "created tower {} from server entity {}",
                                            tower,
                                            aUpdate.Entity);
                                    },
                                    [&](const CreepInitData& aCreepInit) {
                                        // Create creep on client
                                        auto  creep = mRegistry.create();
                                        auto& phy   = mRegistry.ctx().get<Physics>();

                                        auto& transform = mRegistry.emplace<Transform3D>(
                                            creep,
                                            aCreepInit.Position,
                                            glm::identity<glm::quat>(),
                                            glm::vec3(0.5f));

                                        mRegistry.emplace<ModelRotationOffset>(
                                            creep,
                                            glm::angleAxis(
                                                glm::radians(90.0f),
                                                glm::vec3(0, 1, 0)));

                                        // Create physics body and collider
                                        RigidBody body{.Params = aUpdate.Params};
                                        Collider  collider{
                                             .Params =
                                                ColliderParams{
                                                     .CollisionCategoryBits = Category::Entities,
                                                     .CollideWithMaskBits   = Category::Projectiles,
                                                     .IsTrigger             = false,
                                                     .Offset                = Transform3D{},
                                                     .ShapeParams =
                                                        CapsuleShapeParams{
                                                             .Radius = 0.1f,
                                                             .Height = 0.05f,
                                                        },
                                                },
                                        };
                                        body.Body = phy.CreateRigidBody(body.Params, transform);
                                        collider.Handle =
                                            phy.AddCollider(body.Body, collider.Params);

                                        mRegistry.emplace<Creep>(creep, aCreepInit.Type);
                                        mRegistry.emplace<RigidBody>(creep, body);
                                        mRegistry.emplace<Collider>(creep, collider);
                                        mRegistry.emplace<Health>(creep, 100.0f);
                                        mRegistry.emplace<SceneObject>(creep, "phoenix"_hs);
                                        mRegistry.emplace<ImguiDrawable>(creep, "phoenix", true);
                                        mRegistry.emplace<Animator>(creep, 0.0f, "Take 001");

                                        // TODO: Add Path component - need graph context

                                        syncMap.insert_or_assign(aUpdate.Entity, creep);

                                        WATO_INFO(
                                            mRegistry,
                                            "created creep {} from server entity {}",
                                            creep,
                                            aUpdate.Entity);
                                    },
                                    [&](const std::monostate&) {
                                        // Generic entity creation (e.g., terrain colliders)
                                        WATO_INFO(
                                            mRegistry,
                                            "created entity {} (no specific init data)",
                                            aUpdate.Entity);
                                    },
                                },
                                aUpdate.InitData);
                            break;
                        }
                        case RigidBodyEvent::Update: {
                            // Handle entity update
                            if (syncMap.contains(aUpdate.Entity)) {
                                mRegistry.patch<RigidBody>(
                                    syncMap[aUpdate.Entity],
                                    [&aUpdate](RigidBody& aBody) {
                                        aBody.Params = aUpdate.Params;
                                    });
                            }
                            break;
                        }
                        case RigidBodyEvent::Destroy: {
                            // Handle entity destruction
                            if (syncMap.contains(aUpdate.Entity)) {
                                entt::entity clientEntity = syncMap[aUpdate.Entity];
                                mRegistry.destroy(clientEntity);
                                syncMap.erase(aUpdate.Entity);
                                WATO_INFO(
                                    mRegistry,
                                    "destroyed entity {} (server entity {})",
                                    clientEntity,
                                    aUpdate.Entity);
                            }
                            break;
                        }
                    }
                },
                [&](const HealthUpdateResponse& aUpdate) {
                    auto& syncMap = mRegistry.ctx().get<EntitySyncMap>();

                    if (syncMap.contains(aUpdate.Entity)) {
                        entt::entity e = syncMap[aUpdate.Entity];
                        WATO_INFO(
                            mRegistry,
                            "received health update for server {}, updating local {} => {}",
                            aUpdate.Entity,
                            e,
                            aUpdate.Health);
                        mRegistry.patch<Health>(e, [&aUpdate](Health& aHealth) {
                            aHealth.Health = aUpdate.Health;
                        });
                    }
                },
                [&](const std::monostate) {},
            },
            aEvent->Payload);
    });
}
