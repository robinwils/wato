#include <doctest.h>

#include "systems/system.hpp"
#include "test_fixtures.hpp"

TEST_CASE("system.periodic_fixed_default_skips_zero")
{
    using namespace std::chrono_literals;

    struct Counter : public PeriodicFixedSystem {
        using PeriodicFixedSystem::PeriodicFixedSystem;
        const char* Name() const override { return "Counter"; }
        int         Calls = 0;

       protected:
        void PeriodicExecute(Registry&, std::uint32_t) override { ++Calls; }
    };

    Registry reg;
    reg.ctx().emplace<Logger>(WATO_NAMED_LOGGER("test"));

    Counter sys(30s);

    sys.update(0, &reg);
    CHECK_EQ(sys.Calls, 0);

    sys.update(1, &reg);
    CHECK_EQ(sys.Calls, 0);

    sys.update(1800, &reg);
    CHECK_EQ(sys.Calls, 1);
}

TEST_CASE("system.periodic_fixed_fire_immediately")
{
    using namespace std::chrono_literals;

    struct Counter : public PeriodicFixedSystem {
        using PeriodicFixedSystem::PeriodicFixedSystem;
        const char* Name() const override { return "Counter"; }
        int         Calls = 0;

       protected:
        void PeriodicExecute(Registry&, std::uint32_t) override { ++Calls; }
    };

    Registry reg;
    reg.ctx().emplace<Logger>(WATO_NAMED_LOGGER("test"));

    Counter sys(30s, true);

    sys.update(0, &reg);
    CHECK_EQ(sys.Calls, 1);

    sys.update(1, &reg);
    CHECK_EQ(sys.Calls, 1);

    sys.update(1800, &reg);
    CHECK_EQ(sys.Calls, 2);
}

TEST_CASE("system.periodic_frame_accumulator")
{
    using namespace std::chrono_literals;

    struct Counter : public PeriodicFrameSystem {
        using PeriodicFrameSystem::PeriodicFrameSystem;
        const char* Name() const override { return "Counter"; }
        int         Calls = 0;

       protected:
        void PeriodicExecute(Registry&, float) override { ++Calls; }
    };

    Registry reg;
    reg.ctx().emplace<Logger>(WATO_NAMED_LOGGER("test"));

    Counter sys(1s);

    sys.update(0.5f, &reg);
    CHECK_EQ(sys.Calls, 0);

    sys.update(0.5f, &reg);
    CHECK_EQ(sys.Calls, 1);

    sys.update(1.0f, &reg);
    CHECK_EQ(sys.Calls, 2);
}
