#pragma once

#include <bx/bx.h>
#include <reactphysics3d/engine/PhysicsCommon.h>
#include <spdlog/fmt/bin_to_hex.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iterator>
#include <span>
#include <type_traits>

#include "components/health.hpp"
#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "core/physics/physics.hpp"
#include "core/sys/log.hpp"
#include "core/types.hpp"

class BitInputArchive : public StreamDecoder
{
   public:
    BitInputArchive(bit_stream&& aBits, bool aEnableLogger = true) : StreamDecoder(std::move(aBits))
    {
        if (!aEnableLogger) {
            WATO_SER_LOGGER->set_level(spdlog::level::off);
        }
    }

    BitInputArchive(const bit_buffer& aBits, bool aEnableLogger = true) : StreamDecoder(aBits)
    {
        if (!aEnableLogger) {
            WATO_SER_LOGGER->set_level(spdlog::level::off);
        }
    }

    BitInputArchive(uint8_t* aBytes, std::size_t aSize, bool aEnableLogger = true)
        : StreamDecoder(aBytes, aSize)
    {
        if (!aEnableLogger) {
            WATO_SER_LOGGER->set_level(spdlog::level::off);
        }
    }

    BitInputArchive(byte_view aData, bool aEnableLogger = true)
        : StreamDecoder(aData.data(), aData.size())
    {
        if (!aEnableLogger) {
            WATO_SER_LOGGER->set_level(spdlog::level::off);
        }
    }

    void operator()(entt::entity& aEntity)
    {
        ENTT_ID_TYPE entityV;
        if (!DecodeUInt(entityV, 0, std::numeric_limits<uint32_t>::max())) {
            WATO_SER_CRIT("could not decode entity");
            return;
        }

        aEntity = entt::entity(entityV);
        WATO_SER_TRACE("read entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
    }

    void operator()(std::underlying_type_t<entt::entity>& aEntity)
    {
        if (!DecodeUInt(aEntity, 0, std::numeric_limits<uint32_t>::max())) {
            WATO_SER_CRIT("could not read component set size");
            return;
        }
        WATO_SER_TRACE("=====> reading set of size {:d} <=====", aEntity);
    }

    template <typename T>
    void operator()(T& aObj)
    {
        if (!aObj.Archive(*this)) {
            WATO_SER_CRIT("could not read component from archive");
        }
    }
};

class BitOutputArchive : public StreamEncoder
{
   public:
    BitOutputArchive(bool aEnableLogger = true)
    {
        if (!aEnableLogger) {
            WATO_SER_LOGGER->set_level(spdlog::level::off);
        }
    }

    void operator()(entt::entity aEntity)
    {
        ENTT_ID_TYPE entityV = static_cast<ENTT_ID_TYPE>(aEntity);
        WATO_SER_TRACE("writing entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
        EncodeUInt(entityV, 0, std::numeric_limits<uint32_t>::max());
    }

    void operator()(std::underlying_type_t<entt::entity> aEntity)
    {
        WATO_SER_TRACE("<===== writing set of size {:d} =====> ", aEntity);
        EncodeUInt(aEntity, 0, std::numeric_limits<uint32_t>::max());
    }

    template <typename T>
    void operator()(const T& aObj)
    {
        // const_cast is needed because component Archive methods are not const-qualified,
        // but EnTT's snapshot passes const references. This is safe because:
        // 1. BitOutputArchive is detected as IsStreamEncoder at compile-time
        // 2. The Archive helper functions only read from (never modify) the object when encoding
        const_cast<T&>(aObj).Archive(*this);
    }

    bit_buffer&                Data() { return mBits.Data(); }
    std::span<const uint8_t>   Bytes() { return mBits.Bytes(); }
    const std::vector<uint8_t> ByteVector() { return mBits.ByteVector(); }

   private:
};

template <typename OutArchive>
void SaveRegistry(const entt::registry& aRegistry, OutArchive& aArchive)
{
    // TODO: Header with version
    entt::snapshot{aRegistry}
        .template get<entt::entity>(aArchive)
        .template get<Transform3D>(aArchive)
        .template get<Health>(aArchive)
        .template get<RigidBody>(aArchive)
        .template get<Collider>(aArchive);
}

template <typename InArchive>
void LoadRegistry(entt::registry& aRegistry, InArchive& aArchive)
{
    // TODO: Header with version
    entt::snapshot_loader{aRegistry}
        .template get<entt::entity>(aArchive)
        .template get<Transform3D>(aArchive)
        .template get<Health>(aArchive)
        .template get<RigidBody>(aArchive)
        .template get<Collider>(aArchive);
}

