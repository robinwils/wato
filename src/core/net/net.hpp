#pragma once

#include <enet.h>

#include <memory>
#include <variant>

#include "core/event/creep_spawn.hpp"

struct ENetHostDeleter {
    void operator()(ENetHost *aHost) const noexcept
    {
        if (aHost) {
            enet_host_destroy(aHost);
        }
    }
};
using enet_host_ptr = std::unique_ptr<ENetHost, ENetHostDeleter>;

// Events are CRTP classes so there is a concrete type underneath, we need
// to use a variant to represent different event possibilities
using NetEvent = std::variant<CreepSpawnEvent>;
