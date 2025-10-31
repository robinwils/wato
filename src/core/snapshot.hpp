#pragma once

#include <bx/bx.h>
#include <reactphysics3d/engine/PhysicsCommon.h>
#include <spdlog/spdlog.h>

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
#include "core/types.hpp"

class ByteInputArchive
{
   public:
    using byte        = uint8_t;
    using byte_stream = std::span<byte>;

    ByteInputArchive(const byte_stream& aInStream, const Logger& aLogger = spdlog::default_logger())
        : mStorage(aInStream), mIdx(0), mLogger(aLogger)
    {
        mLogger->set_level(spdlog::level::off);
    }
    ByteInputArchive(std::vector<byte> aInVec, const Logger& aLogger = spdlog::default_logger())
        : mStorage(aInVec.begin(), aInVec.size()), mIdx(0), mLogger(aLogger)
    {
    }

    void operator()(entt::entity& aEntity)
    {
        ENTT_ID_TYPE entityV;
        Read<ENTT_ID_TYPE>(&entityV, 1);
        aEntity = static_cast<entt::entity>(entityV);
        mLogger->info("read entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
    }

    void operator()(std::underlying_type_t<entt::entity>& aEntity)
    {
        mLogger->info("=====> reading set of size {:d} <=====", aEntity);
        Read<std::underlying_type_t<entt::entity>>(&aEntity, 1);
    }

    template <typename T>
    void operator()(T& aObj)
    {
        T::Deserialize(*this, aObj);
    }

    byte_stream Bytes() const { return mStorage; }

    void Read(bool& aV)
    {
        ensure(1);
        mLogger->info("reading 1 bool");

        aV = mStorage[mIdx++] != 0;
    }

    template <class T, std::output_iterator<T> Out, std::endian Endian = std::endian::little>
        requires(wire_native<T>)
    void Read(Out aOut, std::size_t aN = 1)
    {
        std::size_t dataSize = aN * sizeof(T);
        ensure(dataSize);

        mLogger->info("reading from buffer of size {}: {::#x}", mStorage.size(), mStorage);
        mLogger->info("reading {} * {} = {} bytes at {}", aN, sizeof(T), dataSize, mIdx);
        if constexpr (std::endian::native == Endian || sizeof(T) == 1) {
            mLogger->info("  native endian");
            std::memcpy(std::to_address(aOut), &mStorage[mIdx], dataSize);

            mIdx += dataSize;
        } else {
            mLogger->info("  swap endian");

            for (std::size_t i = 0; i < aN; ++i, mIdx += sizeof(T)) {
                T v;
                std::memcpy(&v, mStorage.data() + mIdx, sizeof(T));
                *aOut++ = bswap_any(v);
            }
        }
    }

   private:
    void ensure(std::size_t aNeed) const
    {
        auto idx = SafeU32(mIdx) + aNeed;

        idx = idx <= mStorage.size() ? idx : SafeU32(-1);
    }

    byte_stream mStorage;
    uint32_t    mIdx;
    Logger      mLogger;
};

class ByteOutputArchive
{
    using byte        = uint8_t;
    using byte_stream = std::vector<byte>;

   public:
    ByteOutputArchive(const Logger& aLogger = spdlog::default_logger()) : mLogger(aLogger)
    {
        mLogger->set_level(spdlog::level::off);
    }

    void operator()(entt::entity aEntity)
    {
        ENTT_ID_TYPE entityV = static_cast<ENTT_ID_TYPE>(aEntity);
        mLogger->info("writing entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
        Write(&entityV, 1);
    }

    void operator()(std::underlying_type_t<entt::entity> aEntity)
    {
        mLogger->info("<===== writing set of size {:d} =====> ", aEntity);
        Write(&aEntity, 1);
    }

    template <typename T>
    void operator()(const T& aObj)
    {
        T::Serialize(*this, aObj);
    }

    void Write(bool aV)
    {
        mLogger->info("writing 1 byte");
        mStorage.push_back(static_cast<byte>(aV ? 1 : 0));
    }

    template <std::input_iterator In, std::endian Endian = std::endian::little>
        requires(wire_native<std::iter_value_t<In>>)
    void Write(In aSrc, std::size_t aN = 1)
    {
        using T = std::iter_value_t<In>;

        std::size_t dataSize = aN * sizeof(T);
        std::size_t old      = SafeI32(mStorage.size());

        mStorage.resize(mStorage.size() + dataSize);

        mLogger->info("writing {} * {} bytes", aN, sizeof(T));
        if constexpr (std::endian::native == Endian || sizeof(T) == 1) {
            mLogger->info("  native endian");
            std::memcpy(&mStorage[old], std::to_address(aSrc), dataSize);
        } else {
            mLogger->info("  swap endian");
            std::vector<byte> tmp(dataSize);
            for (std::size_t i = 0; i < aN; ++i, ++aSrc) {
                T swapped = bswap_any(*aSrc);
                std::memcpy(tmp.data() + i * sizeof(T), &swapped, sizeof(T));
            }
            std::memcpy(&mStorage[old], tmp.data(), dataSize);
        }
        mLogger->info("wrote to buffer of size {}: {::#x}", mStorage.size(), mStorage);
    }

    byte_stream  Bytes() const { return mStorage; }
    byte_stream& Bytes() { return mStorage; }

   private:
    byte_stream mStorage;
    Logger      mLogger;
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

#include "doctest.h"
TEST_CASE("snapshot.simple")
{
    entt::registry src;

    auto e1 = src.create();
    src.emplace<Transform3D>(e1, glm::vec3(0.0f, 2.0f, 1.5f));

    auto e2 = src.create();
    src.emplace<Transform3D>(
        e2,
        glm::vec3(4.2f, 2.1f, 0.42f),
        glm::identity<glm::quat>(),
        glm::vec3(0.5f));
    src.emplace<Health>(e2, 300.0f);

    std::vector<uint8_t> storage;
    ByteOutputArchive    outAr(storage);
    entt::registry       dest;
    SaveRegistry(src, outAr);

    ByteInputArchive inAr(std::span(outAr.Bytes()));
    LoadRegistry(dest, inAr);

    CHECK(dest.valid(e1));
    CHECK(dest.valid(e2));
    CHECK_EQ(dest.get<Transform3D>(e1).Position, glm::vec3(0.0f, 2.0f, 1.5f));
    CHECK_EQ(dest.get<Health>(e2).Health, 300.0f);
}
