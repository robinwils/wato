#pragma once

#include "registry/registry.hpp"

class WatoWindow;

class MenuBackend
{
   public:
    virtual ~MenuBackend() = default;

    virtual void Render(Registry& aReg) = 0;
};
