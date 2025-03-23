#pragma once

#include <enet.h>

#include <memory>

struct ENetHostDeleter {
    void operator()(ENetHost *aHost) const noexcept
    {
        if (aHost) {
            enet_host_destroy(aHost);
        }
    }
};
using enet_host_ptr = std::unique_ptr<ENetHost, ENetHostDeleter>;
