#pragma once

#include <entt/entity/organizer.hpp>

#include "components/scene_object.hpp"
#include "components/transform3d.hpp"
#include "core/physics/physics.hpp"
#include "registry/registry.hpp"
#include "systems/system.hpp"

class RigidBodiesUpdateSystem : public System<RigidBodiesUpdateSystem>
{
   public:
    using view_type = entt::view<entt::get_t<const SceneObject, const Transform3D>>;

    void Register(entt::organizer& aOrganizer)
    {
        aOrganizer.emplace<&RigidBodiesUpdateSystem::operator()>(*this, StaticName());
    }
    void operator()(Registry& aRegistry, const float aDeltaTime);

    static constexpr const char* StaticName() { return "RigidBodiesUpdateSystem"; }
};
