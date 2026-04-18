#include <input/action.hpp>

#include "systems/action.hpp"
#include "test_fixtures.hpp"

TEST_CASE("action.encode")
{
    StreamEncoder enc;
    Action        ae1 = kBuildTowerAction;
    Action        ae2 = kSendCreepAction;

    ae1.Archive(enc);
    ae2.Archive(enc);

    StreamDecoder dec(enc.Data());
    Action        ad1;
    Action        ad2;

    CHECK_EQ(ad1.Archive(dec), true);
    CHECK_EQ(ad1, ae1);

    CHECK_EQ(ad2.Archive(dec), true);
    CHECK_EQ(ad2, ae2);
}

TEST_CASE_FIXTURE(ClientFixture, "action.client.build_no_placement")
{
    DeterministicActionSystem sys;
    AddPlayer(0, 0);

    Buf.Latest().Actions.push_back(kBuildTowerAction);
    sys.update(0, &Reg);

    auto g = Reg.group<Tower>(entt::get<Collider, RigidBody>);
    CHECK_EQ(0, g.size());
}

TEST_CASE_FIXTURE(ClientFixture, "action.client.build_placement")
{
    DeterministicActionSystem sys;
    AddPlayer(0, 0);

    Ctx.EnterPlacement(Reg, TowerType::Arrow);
    Buf.Latest().Actions.push_back(kBuildTowerAction);
    sys.update(0, &Reg);

    auto g = Reg.group<Tower>(entt::get<Collider, RigidBody>);
    CHECK_EQ(0, g.size());
}

TEST_CASE_FIXTURE(ServerFixture, "action.server.build")
{
    ServerActionSystem sys;
    AddPlayer(0, 0);
    AddPlayerGraph(0);

    Tagged.push_back({0, kBuildTowerAction});
    sys.update(0, &Reg);

    auto g = Reg.group<Tower>(entt::get<Collider, RigidBody>);
    CHECK_EQ(1, g.size());
}

TEST_CASE_FIXTURE(ServerFixture, "action.server.build_deducts_gold")
{
    ServerActionSystem sys;
    auto               player = AddPlayer(0, 0);
    AddPlayerGraph(0);

    auto& gold      = Reg.get<Gold>(player);
    int   initial   = gold.Balance;
    int   towerCost = Definitions.Towers.at(TowerType::Arrow).Cost;

    Tagged.push_back({0, kBuildTowerAction});
    sys.update(0, &Reg);

    CHECK_EQ(gold.Balance, initial - towerCost);
}

TEST_CASE_FIXTURE(ServerFixture, "action.server.build_insufficient_gold")
{
    ServerActionSystem sys;
    auto               player = AddPlayer(0, 0, 10);  // only 10 gold
    AddPlayerGraph(0);

    Tagged.push_back({0, kBuildTowerAction});
    sys.update(0, &Reg);

    auto g = Reg.group<Tower>(entt::get<Collider, RigidBody>);
    CHECK_EQ(g.size(), 0);
    CHECK_EQ(Reg.get<Gold>(player).Balance, 10);
}

TEST_CASE_FIXTURE(ServerFixture, "action.server.build_same_pos")
{
    ServerActionSystem sys;
    AddPlayer(0, 0);
    AddPlayerGraph(0);

    Tagged.push_back({0, kBuildTowerAction});
    Tagged.push_back({0, kBuildTowerAction});
    sys.update(0, &Reg);

    auto g = Reg.group<Tower>(entt::get<Collider, RigidBody>);

    // tower invalidated
    CHECK_EQ(1, g.size());
}

TEST_CASE_FIXTURE(ServerFixture, "action.server.send_deducts_gold")
{
    ServerActionSystem sys;
    auto               sender = AddPlayer(0, 0);
    AddPlayer(1, 1);
    AddPlayerGraph(1);
    AddSpawner(1, 1);

    auto& gold    = Reg.get<Gold>(sender);
    int   initial = gold.Balance;
    int   cost    = Definitions.Creeps.at(CreepType::Simple).Cost;

    Tagged.push_back({0, kSendCreepAction});
    sys.update(0, &Reg);

    CHECK_EQ(gold.Balance, initial - cost);
}

TEST_CASE_FIXTURE(ServerFixture, "action.server.send_insufficient_gold")
{
    ServerActionSystem sys;
    auto               sender = AddPlayer(0, 0, 5);
    AddPlayer(1, 1);
    AddPlayerGraph(1);
    AddSpawner(1, 1);

    Tagged.push_back({0, kSendCreepAction});
    sys.update(0, &Reg);

    CHECK_EQ(Reg.get<Gold>(sender).Balance, 5);
    CHECK_EQ(Reg.view<Creep>().size(), 0);
}
