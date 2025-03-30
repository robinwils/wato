#pragma once

#include <entt/entity/organizer.hpp>

#include "components/light_source.hpp"
#include "components/placement_mode.hpp"
#include "components/scene_object.hpp"
#include "components/transform3d.hpp"
#include "config.h"
#include "core/physics.hpp"
#include "registry/registry.hpp"
#include "systems/system.hpp"

class RenderSystem : public System<RenderSystem>
{
   public:
    using view_type = entt::view<entt::get_t<const SceneObject, const Transform3D>>;

    void Register(entt::organizer& aOrganizer)
    {
        aOrganizer.emplace<&RenderSystem::operator(), const LightSource, const PlacementMode>(*this,
            StaticName());
    }
    void operator()(Registry& aRegistry, const float aDeltaTime);

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

#if WATO_DEBUG
class PhysicsDebugSystem : public System<PhysicsDebugSystem>
{
   public:
    void Register(entt::organizer& aOrganizer)
    {
        aOrganizer.emplace<&PhysicsDebugSystem::operator(), Physics>(*this, StaticName());
    }
    void operator()(Registry& aRegistry, const float aDeltaTime);

    static constexpr const char* StaticName() { return "PhysicsDebugSystem "; }
};
#endif
