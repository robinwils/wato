#pragma once

#include "entt/signal/delegate.hpp"
#include "registry/registry.hpp"

using SystemDelegate = entt::delegate<void(Registry&, const float)>;
using SystemRegistry = std::vector<SystemDelegate>;

template <typename Derived>
class System
{
   public:
    void Run(Registry& aRegistry, const float aDeltaTime)
    {
        (static_cast<Derived*>(this))(aRegistry, aDeltaTime);
    }

    static SystemDelegate MakeDelegate(Derived& aInstance)
    {
        SystemDelegate del;

        del.connect<&Derived::operator()>(aInstance);
        return del;
    }

    constexpr const char* Name() const { return Derived::StaticName(); }
};
