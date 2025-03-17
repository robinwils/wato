#pragma once

#include "core/registry.hpp"
#include "reactphysics3d/reactphysics3d.h"
#include "systems/systems.hpp"

class EventHandler : public rp3d::EventListener
{
   public:
    EventHandler(Registry& m_registry, ActionSystem& m_action_system)
        : m_registry(m_registry), m_action_system(m_action_system)
    {
    }
    /// Called when some contacts occur
    /**
     * @param callbackData Contains information about all the contacts
     */
    virtual void onContact(const rp3d::CollisionCallback::CallbackData& /*callbackData*/) override;

    /// Called when some trigger events occur
    /**
     * @param callbackData Contains information about all the triggers that are colliding
     */
    virtual void onTrigger(const rp3d::OverlapCallback::CallbackData& /*callbackData*/) override;

   private:
    Registry&     m_registry;
    ActionSystem& m_action_system;
};
