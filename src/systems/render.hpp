#pragma once

#include "registry/registry.hpp"
#include "systems/system.hpp"

class RenderSystem : public System<RenderSystem>
{
   public:
    void operator()(Registry& aRegistry);

    static constexpr const char* StaticName() { return "RenderSystem"; }
};

class RenderImguiSystem : public System<RenderImguiSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime);

    static constexpr const char* StaticName() { return "RenderImguiSystem"; }
};

class CameraSystem : public System<CameraSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime);

    static constexpr const char* StaticName() { return "CameraSystem"; }
};
