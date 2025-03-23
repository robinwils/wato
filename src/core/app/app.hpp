#pragma once

#include "core/event_handler.hpp"
#include "registry/registry.hpp"
#include "systems/physics.hpp"
#include "systems/system.hpp"

class Application
{
   public:
    explicit Application(int aWidth, int aHeight) : mWidth(aWidth), mHeight(aHeight) {}
    virtual ~Application() = default;

    Application(const Application &)            = delete;
    Application(Application &&)                 = delete;
    Application &operator=(const Application &) = delete;
    Application &operator=(Application &&)      = delete;

    virtual void Init() = 0;
    virtual int  Run()  = 0;

   protected:
    Registry mRegistry;

    PhysicsSystem mPhysicsSystem;

    SystemRegistry mSystems;

    int mWidth, mHeight;
};
