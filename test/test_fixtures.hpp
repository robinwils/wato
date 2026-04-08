#pragma once

#include <doctest.h>

#include <glaze/glaze.hpp>

#include "components/player.hpp"
#include "components/rigid_body.hpp"
#include "components/tower.hpp"
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

    entt::entity AddPlayer(PlayerID aID, uint8_t aSlot)
    {
        auto e = Reg.create();
        Reg.emplace<Player>(e, aID, aSlot);
        return e;
    }

    template <typename... Systems>
    void RegisterFixedSystems()
    {
        (FixedExec.Register<Systems>(), ...);
    }

    void Tick(uint32_t aTick = 0) { FixedExec.Update(aTick, &Reg); }
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
    TaggedActionsType& Tagged = Reg.ctx().emplace<TaggedActionsType>();
    PlayerGraphMap&    Graphs = Reg.ctx().emplace<PlayerGraphMap>();

    void AddPlayerGraph(PlayerID aID, float aMapSize = 20.0f)
    {
        Graphs.try_emplace(
            aID,
            aMapSize * GraphCell::kCellsPerAxis,
            aMapSize * GraphCell::kCellsPerAxis,
            glm::vec2{0.0f, 0.0f});
    }
};
