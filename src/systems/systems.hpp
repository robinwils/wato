#pragma once
#include <core/registry.hpp>

#include "core/action.hpp"
#include "entt/entity/entity.hpp"

void renderSceneObjects(Registry& registry, const float dt);
void cameraSystem(Registry& registry, float width, float height);
void processInputs(Registry& registry, const double time_delta);
void renderImgui(Registry& registry, float width, float height);
void physicsSystem(Registry& registry, double delta_time);
void physicsDebugRenderSystem(Registry& registry);

struct ActionSystem {
   public:
    ActionSystem(Registry& _r, int _w, int _h) : m_registry(_r), m_win_width(_w), m_win_height(_h)
    {
    }

    void init_listeners();
    void udpate_win_size(int _w, int _h);

   private:
    glm::vec3 get_mouse_ray() const;

    // handlers
    void camera_movement(CameraMovement);
    void build_tower(BuildTower);
    void tower_placement_mode(TowerPlacementMode);

    Registry&    m_registry;
    entt::entity m_ghost_tower{entt::null};
    float        m_win_width, m_win_height;
};
