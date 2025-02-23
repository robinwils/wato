#pragma once
#include <core/registry.hpp>

#include "core/action.hpp"

void renderSceneObjects(Registry& registry, const float dt);
void cameraSystem(Registry& registry, float width, float height);
void processInputs(Registry& registry, double time_delta);
void renderImgui(Registry& registry, float width, float height);

struct ActionSystem {
   public:
    ActionSystem(Registry& _r) : m_registry(_r) {}

    void init_listeners();

   private:
    void camera_movement(CameraMovement);

    Registry& m_registry;
};
