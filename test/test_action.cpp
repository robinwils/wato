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
