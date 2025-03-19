#pragma once
#include <bgfx/bgfx.h>
#include <bimg/decode.h>

#include <entt/entity/registry.hpp>

class Registry : public entt::basic_registry<entt::entity>
{
   public:
    void SpawnPlane();
    void SpawnMap(uint32_t aWidth, uint32_t aHeight);
    void SpawnLight();
    void LoadShaders();
    void LoadModels();
    void SpawnPlayerAndCamera();
};
