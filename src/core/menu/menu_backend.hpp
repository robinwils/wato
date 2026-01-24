#pragma once

#include "registry/registry.hpp"

class WatoWindow;

class MenuBackend
{
   public:
    virtual ~MenuBackend();
    virtual void Render(const Registry& aReg, const WatoWindow& aWin) = 0;
};
