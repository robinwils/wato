#include "core/app/app.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "components/game.hpp"
#include "components/scene_object.hpp"
#include "components/spawner.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/event_handler.hpp"
#include "core/graph.hpp"
#include "core/physics.hpp"
#include "input/action.hpp"

using namespace entt::literals;

void Application::Init()
{
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto fileSink    = std::make_shared<spdlog::sinks::basic_file_sink_mt>("wato_logs.txt", true);
    spdlog::level::level_enum level = spdlog::level::from_str(mOptions.LogLevel());

    consoleSink->set_level(level);
    fileSink->set_level(level);
    // FIXME: weird segfault when using %s and %# instead of %@
    // or puting the thread info in separate []
    consoleSink->set_pattern("[%H:%M:%S %z thread %t] [%^%L%$] %v %@");
    fileSink->set_pattern("[%H:%M:%S %z thread %t] [%^%L%$] %v %@");

    std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
    auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
    spdlog::set_default_logger(logger);
    logger->set_level(level);
    logger->warn("this should appear in both console and file");
    logger->info("this message should not appear in the console, only in the file");
}

void Application::StartGameInstance(
    Registry&            aRegistry,
    const GameInstanceID aGameID,
    const bool           aIsServer)
{
    spdlog::info("spawning game instance");
    auto& physics = aRegistry.ctx().emplace<Physics>();
    auto& stack   = aRegistry.ctx().emplace<ActionContextStack>();

    aRegistry.ctx().emplace<ActionBuffer>();
    aRegistry.ctx().emplace<GameInstance>(aGameID, 0.0f, 0u);

    physics.Init(aRegistry);
    // TODO: leak ?
    physics.World()->setEventListener(new EventHandler(&aRegistry));

    enum ActionContext::State actionContextState = ActionContext::State::Default;
    if (aIsServer) {
        actionContextState = ActionContext::State::Server;
    }
    stack.push_back(ActionContext{
        .State    = actionContextState,
        .Bindings = ActionBindings::Defaults(),
        .Payload  = NormalPayload{}});
    SpawnMap(aRegistry, 20, 20);
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

    ClearAllObservers(aRegistry);
}

void Application::SpawnMap(Registry& aRegistry, uint32_t aWidth, uint32_t aHeight)
{
    auto&        physics = aRegistry.ctx().get<Physics>();
    entt::entity first   = entt::null;

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
    auto& baseTransform = aRegistry.emplace<Transform3D>(base, glm::vec3(10.0f, 0.0f, 10.0f));
    aRegistry.emplace<Base>(base);

    graph.ComputePaths(GraphCell::FromWorldPoint(baseTransform.Position));

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
    glm::vec3        translate = glm::vec3(aWidth + 1, 1.004f, aHeight + 1) / 4.0f - 0.25f;
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
