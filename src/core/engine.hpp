#pragma once

#include <memory>

#include "core/physics.hpp"
#include "core/window.hpp"
#include "renderer/renderer.hpp"

class Engine
{
   public:
    Engine() {}
    Engine(std::unique_ptr<WatoWindow> aWin,
        std::unique_ptr<Renderer>      aRenderer,
        std::unique_ptr<Physics>       aPhy)
        : mWindow(std::move(aWin)), mRenderer(std::move(aRenderer)), mPhysics(std::move(aPhy))
    {
    }
    Engine(Engine &&)                 = delete;
    Engine(const Engine &)            = delete;
    Engine &operator=(Engine &&)      = delete;
    Engine &operator=(const Engine &) = delete;
    ~Engine()                         = default;

    Input      &GetPlayerInput() { return mWindow->GetInput(); }
    Physics    &GetPhysics() { return *mPhysics; }
    Renderer   &GetRenderer() { return *mRenderer; }
    WatoWindow &GetWindow() { return *mWindow; }

   private:
    std::unique_ptr<WatoWindow> mWindow;
    std::unique_ptr<Renderer>   mRenderer;
    std::unique_ptr<Physics>    mPhysics;
};
