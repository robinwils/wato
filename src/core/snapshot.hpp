#pragma once

#include <bx/bx.h>
#include <reactphysics3d/engine/PhysicsCommon.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <entt/entt.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iterator>
#include <span>

#include "components/health.hpp"
#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "core/physics/physics.hpp"
#include "core/types.hpp"

class ByteInputArchive
{
    using byte        = uint8_t;
    using byte_stream = std::span<byte>;

   public:
    ByteInputArchive(const byte_stream& aInStream) : mStorage(aInStream), mIdx(0) {}

    void operator()(entt::entity& aEntity)
    {
        Read<entt::entity>(&aEntity, 1);
        spdlog::debug("read entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
    }

    void operator()(std::underlying_type_t<entt::entity>& aEntity)
    {
        Read<std::underlying_type_t<entt::entity>>(&aEntity, 1);
        spdlog::debug("read set of size {:d}", aEntity);
    }

    template <typename T>
    void operator()(T& aObj)
    {
        T::Deserialize(*this, aObj);
    }

    byte_stream Bytes() const { return mStorage; }
    template <typename T, std::output_iterator<T> Out>
    void Read(Out aDestination, std::size_t aN)
    {
        SafeU32 idx = SafeU32(mIdx) + aN * sizeof(T);

        idx = idx < mStorage.size() ? idx : SafeU32(-1);
        spdlog::debug(
            "reading {:d} elts [size  = {:d}] at index {} [total size = {}]",
            aN,
            sizeof(T),
            static_cast<uint32_t>(idx),
            aN * sizeof(T));

        // bx::memCopy(aDestination, &mStorage[mIdx], aN * sizeof(T));
        std::copy_n(reinterpret_cast<const T*>(&mStorage[mIdx]), aN, aDestination);
        mIdx = idx;
    }

   private:
    byte_stream mStorage;
    uint32_t    mIdx;
};

class ByteOutputArchive
{
    using byte        = uint8_t;
    using byte_stream = std::vector<byte>;

   public:
    ByteOutputArchive() = default;
    explicit ByteOutputArchive(byte_stream& aOutStream) : mStorage(aOutStream) {}

    void operator()(entt::entity aEntity)
    {
        spdlog::debug("writing entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
        Write<entt::entity>(&aEntity, 1);
    }

    void operator()(std::underlying_type_t<entt::entity> aEntity)
    {
        spdlog::debug("writing set of size {:d}", aEntity);
        Write<std::underlying_type_t<entt::entity>>(&aEntity, 1);
    }

    template <typename T>
    void operator()(const T& aObj)
    {
        T::Serialize(*this, aObj);
    }

    template <typename T, std::input_iterator In>
    void Write(const In& aData, std::size_t aN)
    {
        spdlog::debug("writing {} elts [size = {}, total = {}]", aN, sizeof(T), aN * sizeof(T));
        auto* p = reinterpret_cast<const byte*>(aData);
        mStorage.insert(mStorage.end(), p, p + aN * sizeof(T));
    }

    byte_stream  Bytes() const { return mStorage; }
    byte_stream& Bytes() { return mStorage; }

   private:
    byte_stream mStorage;
};

template <typename OutArchive>
void SaveRegistry(const entt::registry& aRegistry, OutArchive& aArchive)
{
    // TODO: Header with version
    entt::snapshot{aRegistry}
        .template get<entt::entity>(aArchive)
        .template get<Transform3D>(aArchive)
        .template get<Health>(aArchive);
}

template <typename InArchive>
void LoadRegistry(entt::registry& aRegistry, InArchive& aArchive)
{
    // TODO: Header with version
    entt::snapshot_loader{aRegistry}
        .template get<entt::entity>(aArchive)
        .template get<Transform3D>(aArchive)
        .template get<Health>(aArchive);
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
