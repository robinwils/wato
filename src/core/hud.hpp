#pragma once

#include "registry/registry.hpp"

class WatoWindow;

class HeadsUpDisplay
{
   public:
    virtual ~HeadsUpDisplay() {}
    virtual void Render(const Registry& aReg, const WatoWindow& aWin) = 0;
};
