#pragma once

#include <bimg/decode.h>

#include <entt/entity/registry.hpp>

#include "registry/registry.hpp"

class EventHandler;

void LoadResources(Registry& aRegistry);
void LoadShaders(Registry& aRegistry);
void SpawnLight(Registry& aRegistry);
void LoadTextures(Registry& aRegistry, uint32_t aW, uint32_t aH);
void LoadModels();
