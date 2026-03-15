#include <doctest.h>

#include <core/physics/physics.hpp>
#include <registry/registry.hpp>

TEST_CASE("physics.init_and_delete_from_registry")
{
    Logger   logger = WATO_NAMED_LOGGER("tmp");
    Registry reg;
    auto&    phy = reg.ctx().emplace<Physics>(logger);
    phy.Init();

    reg.ctx().erase<Physics>();
}
