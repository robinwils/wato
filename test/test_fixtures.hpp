#pragma once

#include <doctest.h>

#include <glaze/glaze.hpp>

#include "components/creep.hpp"
#include "components/game.hpp"
#include "components/health.hpp"
#include "components/player.hpp"
#include "components/rigid_body.hpp"
#include "components/spawner.hpp"
#include "components/tower.hpp"
#include "components/transform3d.hpp"
#include "core/gameplay_definitions.hpp"
#include "core/graph.hpp"
#include "core/physics/physics.hpp"
#include "core/state.hpp"
#include "core/sys/log.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"
#include "systems/system_executor.hpp"

struct RegistryFixture {
    Logger               Log = WATO_NAMED_LOGGER("test");
    Registry             Reg;
    Physics&             Phy       = Reg.ctx().emplace<Physics>(Log);
    GameStateBuffer&     Buf       = Reg.ctx().emplace<GameStateBuffer>();
    FixedSystemExecutor& FixedExec = Reg.ctx().emplace<FixedSystemExecutor>();
    GameInstance&        Instance  = Reg.ctx().emplace<GameInstance>();
    GameplayDef          Definitions;

    RegistryFixture()
    {
        Log->set_level(spdlog::level::trace);
        Reg.ctx().emplace<Logger>(Log);

        auto err = glz::read_file_json(Definitions, TESTDATA_DIR "/gameplay.json", std::string{});
        if (err) {
            Log->critical(glz::format_error(err));
        }
        Reg.ctx().emplace<const GameplayDef&>(Definitions);
        Reg.group<Tower>(entt::get<Collider, RigidBody>);
        Phy.Init();
    }

    entt::entity AddPlayer(PlayerID aID, uint8_t aSlot, int aGold = -1, float aHealth = 10.0f)
    {
        auto player = Reg.create();
        Reg.emplace<Player>(player, aID, aSlot);
        Reg.emplace<Gold>(player, aGold >= 0 ? aGold : Definitions.Economy.StartingGold);
        Reg.emplace<Health>(player, aHealth);
        Reg.emplace<Transform3D>(player, glm::vec3(2.0f, 0.0f, 2.0f));
        return player;
    }

    template <typename... Systems>
    void RegisterFixedSystems()
    {
        (FixedExec.Register<Systems>(), ...);
    }

    void RunTicks(uint32_t aCount = 1)
    {
        for (uint32_t i = 0; i < aCount; ++i) {
            ++Instance.Tick;
            FixedExec.Update(Instance.Tick, &Reg);
        }
    }
};

struct ClientFixture : RegistryFixture {
    ActionContextStack& Ctx      = Reg.ctx().emplace<ActionContextStack>();
    FrameActionBuffer&  FrameBuf = Reg.ctx().emplace<FrameActionBuffer>();
    FrameSystemExecutor FrameExec;
    ::Graph&            Graph;

    explicit ClientFixture(float aMapSize = 20.0f)
        : Graph(Reg.ctx().emplace<::Graph>(
              aMapSize * GraphCell::kCellsPerAxis,
              aMapSize * GraphCell::kCellsPerAxis,
              glm::vec2{0.0f, 0.0f}))
    {
    }

    template <typename... Systems>
    void RegisterFrameSystems()
    {
        (FrameExec.Register<Systems>(), ...);
    }

    void Frame(float aDelta = 1.0f / 60.0f) { FrameExec.Update(aDelta, &Reg); }
};

struct ServerFixture : RegistryFixture {
    TaggedActionsType&     Tagged  = Reg.ctx().emplace<TaggedActionsType>();
    PlayerGraphMap&        Graphs  = Reg.ctx().emplace<PlayerGraphMap>();
    CommonIncome&          Income  = Reg.ctx().emplace<CommonIncome>(Definitions.Economy.StartingIncome);
    std::vector<PlayerID>& Ranking = Reg.ctx().emplace_as<std::vector<PlayerID>>("ranking"_hs);

    void AddPlayerGraph(PlayerID aID, float aMapSize = 20.0f)
    {
        Graphs.try_emplace(
            aID,
            aMapSize * GraphCell::kCellsPerAxis,
            aMapSize * GraphCell::kCellsPerAxis,
            glm::vec2{0.0f, 0.0f});
    }

    entt::entity AddSpawner(PlayerID aOwnerID, uint8_t aOwnerSlot)
    {
        auto spawner = Reg.create();
        Reg.emplace<Spawner>(spawner);
        Reg.emplace<Owner>(spawner, aOwnerID, aOwnerSlot);
        Reg.emplace<Transform3D>(spawner, glm::vec3(5.0f, 0.0f, 5.0f));
        return spawner;
    }

    entt::entity AddCreep(CreepType aType, float aHealth, PlayerID aLastHitBy = PlayerID{})
    {
        auto creep = Reg.create();
        Reg.emplace<Health>(creep, Health{.Health = aHealth, .LastHitBy = aLastHitBy});
        Reg.emplace<Creep>(creep, aType, Definitions.Creeps.at(aType).Damage);
        return creep;
    }
};
