#include <doctest.h>

#include "systems/economy.hpp"
#include "systems/health.hpp"
#include "test_fixtures.hpp"

TEST_CASE_FIXTURE(ServerFixture, "economy.kill_reward")
{
    auto player = AddPlayer(1, 0);

    auto& gold    = Reg.get<Gold>(player);
    int   initial = gold.Balance;
    int   reward  = Definitions.Creeps.at(CreepType::Simple).KillReward;

    AddCreep(CreepType::Simple, 0.0f, 1);  // dead, killed by player 1

    HealthSystem sys;
    sys.update(1, &Reg);

    CHECK_EQ(gold.Balance, initial + reward);
    CHECK_EQ(Reg.view<Creep>().size(), 0);
}

TEST_CASE_FIXTURE(ServerFixture, "economy.no_reward_on_leak")
{
    auto player = AddPlayer(1, 0);

    auto& gold    = Reg.get<Gold>(player);
    int   initial = gold.Balance;

    AddCreep(CreepType::Simple, 0.0f);  // dead, no LastHitBy

    HealthSystem sys;
    sys.update(1, &Reg);

    CHECK_EQ(gold.Balance, initial);
    CHECK_EQ(Reg.view<Creep>().size(), 0);
}

TEST_CASE_FIXTURE(ServerFixture, "economy.redistribution")
{
    using namespace std::chrono_literals;

    auto p1 = AddPlayer(1, 0);
    auto p2 = AddPlayer(2, 1);

    auto& gold1   = Reg.get<Gold>(p1);
    auto& gold2   = Reg.get<Gold>(p2);
    int   initial = gold1.Balance;
    int   income  = Income.Value;

    FixedExec.Register<EconomySystem>(30s);
    RunTicks(1800);

    CHECK_EQ(gold1.Balance, initial + income);
    CHECK_EQ(gold2.Balance, initial + income);
}
