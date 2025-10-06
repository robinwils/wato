#pragma once

#include <entt/entity/organizer.hpp>

#include "components/light_source.hpp"
#include "components/placement_mode.hpp"
#include "components/scene_object.hpp"
#include "components/transform3d.hpp"
#include "core/physics/physics.hpp"
#include "registry/registry.hpp"
#include "systems/system.hpp"

class AiSystem : public System<AiSystem>
{
   public:
    using view_type = entt::view<entt::get_t<const SceneObject, const Transform3D>>;

    void Register(entt::organizer& aOrganizer)
    {
        aOrganizer.emplace<&AiSystem::operator()>(*this, StaticName());
    }
    void operator()(Registry& aRegistry);

    static constexpr const char* StaticName() { return "AiSystem"; }
};
