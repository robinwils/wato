#pragma once
#include <bgfx/bgfx.h>
#include <bimg/decode.h>

#include <entt/entity/registry.hpp>

struct Registry : public entt::basic_registry<entt::entity> {
    void spawnPlane();
    void spawnMap(uint32_t _w, uint32_t _h);
    void spawnLight();
    void loadModels();
    void spawnPlayerAndCamera();
};
