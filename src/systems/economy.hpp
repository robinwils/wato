#pragma once

#include "systems/system.hpp"

class EconomySystem : public PeriodicFixedSystem
{
   public:
    using PeriodicFixedSystem::PeriodicFixedSystem;

    const char* Name() const override { return "EconomySystem"; }

   protected:
    void PeriodicExecute(Registry& aRegistry, std::uint32_t aTick) override;
};
