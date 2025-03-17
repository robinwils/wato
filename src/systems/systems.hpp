#pragma once
#include <core/registry.hpp>

#include "config.h"
#include "core/action.hpp"
#include "entt/entity/entity.hpp"
#include "input/input.hpp"
#include "reactphysics3d/reactphysics3d.h"

void renderSceneObjects(Registry& registry, const float dt);
void cameraSystem(Registry& registry, float width, float height);
void processInputs(Registry& registry, const double time_delta);
void renderImgui(Registry& registry, float width, float height);
void physicsSystem(Registry& registry, double delta_time);

#if WATO_DEBUG
void physicsDebugRenderSystem(Registry& registry);
#endif

struct ActionSystem {
   public:
    ActionSystem(Registry& _r, int _w, int _h) : m_registry(_r), m_win_width(_w), m_win_height(_h)
    {
    }

    void init_listeners(Input& input);
    void udpate_win_size(int _w, int _h);
    void setCanBuild(bool can_build) { m_can_build = can_build; }

   private:
    glm::vec3 get_mouse_ray() const;

    // handlers
    void camera_movement(CameraMovement);
    void build_tower(BuildTower);
    void tower_placement_mode(TowerPlacementMode);

    Registry&    m_registry;
    entt::entity m_ghost_tower{entt::null};
    float        m_win_width, m_win_height;
    bool         m_can_build = true;
};
