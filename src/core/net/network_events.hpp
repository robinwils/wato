#pragma once

#include "core/net/net.hpp"
#include "registry/registry.hpp"

/**
 * @brief Event types for EnTT dispatcher
 *
 * These wrap network response types for deferred processing.
 * Events are enqueued at frame time and processed at fixed timestep.
 * Registry pointer is included so handlers can access the ECS.
 */

struct NewGameEvent {
    Registry*       Reg;
    NewGameResponse Response;
};

struct RigidBodyUpdateEvent {
    Registry*               Reg;
    RigidBodyUpdateResponse Response;
};

struct HealthUpdateEvent {
    Registry*            Reg;
    HealthUpdateResponse Response;
};

struct SyncPayloadEvent {
    Registry*   Reg;
    SyncPayload Payload;
};
