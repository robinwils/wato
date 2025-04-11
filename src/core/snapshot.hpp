#pragma once

#include <bx/bx.h>
#include <reactphysics3d/engine/PhysicsCommon.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <iterator>
#include <stdexcept>

#include "components/health.hpp"
#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "core/physics.hpp"
#include "fmt/base.h"
#include "fmt/format.h"
#include "registry/registry.hpp"

class ByteInputArchive
{
    using byte        = uint8_t;
    using byte_stream = std::vector<byte>;

   public:
    ByteInputArchive(byte_stream& aInStream) : mStorage(aInStream), mIdx(0) {}

    void operator()(entt::entity& aEntity)
    {
        Read<entt::entity>(&aEntity, 1);
        // fmt::println("in entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
    }

    void operator()(std::underlying_type_t<entt::entity>& aEntity)
    {
        Read<std::underlying_type_t<entt::entity>>(&aEntity, 1);
        // fmt::println("in underlying type entity {:d}", aEntity);
    }

    template <typename T>
    void operator()(T& aObj)
    {
        T::Deserialize(*this, aObj);
        // fmt::println("in transform");
    }

    byte_stream Bytes() const { return mStorage; }
    template <typename T, std::output_iterator<T> Out>
    void Read(Out aDestination, std::size_t aN)
    {
        if (mIdx + aN * sizeof(T) > mStorage.size()) {
            throw std::out_of_range(fmt::format(" mIdx = {:d}, asked = {:d}, size = {:d}",
                mIdx,
                aN * sizeof(T),
                mStorage.size()));
        }

        // fmt::println("reading {:d} elts with type size {:d}", aN, sizeof(T));
        // bx::memCopy(aDestination, &mStorage[mIdx], aN * sizeof(T));
        std::copy_n(reinterpret_cast<const T*>(&mStorage[mIdx]), aN, aDestination);
        mIdx += aN * sizeof(T);
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
    ByteOutputArchive(byte_stream& aOutStream) : mStorage(aOutStream) {}

    void operator()(entt::entity aEntity)
    {
        fmt::println("out entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
        Write<entt::entity>(&aEntity, 1);
    }

    void operator()(std::underlying_type_t<entt::entity> aEntity)
    {
        fmt::println("out underlying type entity {:d}", aEntity);
        Write<std::underlying_type_t<entt::entity>>(&aEntity, 1);
    }

    template <typename T>
    void operator()(const T& aObj)
    {
        fmt::println("got a component of size {:d}", sizeof(T));
        T::Serialize(*this, aObj);
    }

    template <typename T, std::input_iterator In>
    void Write(const In& aData, std::size_t aN)
    {
        auto* p = reinterpret_cast<const byte*>(aData);
        // std::copy(p, p + aN, mStorage.end());
        mStorage.insert(mStorage.end(), p, p + aN * sizeof(T));
    }

    byte_stream  Bytes() const { return mStorage; }
    byte_stream& Bytes() { return mStorage; }

   private:
    byte_stream mStorage;
};

template <typename OutArchive>
void SaveRegistry(const entt::registry& aRegistry, OutArchive& aArchive);

template <typename InArchive>
void LoadRegistry(entt::registry& aRegistry, InArchive& aArchive);

#include "doctest.h"
TEST_CASE("snapshot.simple")
{
    entt::registry src;
    auto&          phy = src.ctx().emplace<Physics>();
    phy.Init();

    auto e1 = src.create();
    src.emplace<Transform3D>(e1, glm::vec3(0.0f, 2.0f, 1.5f), glm::vec3(1.0f), glm::vec3(1.0f));

    auto e2 = src.create();
    src.emplace<Transform3D>(e2, glm::vec3(4.2f, 2.1f, 0.42f), glm::vec3(42.0f), glm::vec3(0.5f));
    src.emplace<Health>(e2, 300.0f);

    auto e3 = src.create();
    phy.CreateRigidBody(e3,
        src,
        RigidBodyParams{.Type = rp3d::BodyType::STATIC,
            .Transform        = rp3d::Transform::identity(),
            .GravityEnabled   = false});

    std::vector<uint8_t> storage;
    ByteOutputArchive    outAr(storage);
    entt::registry       dest;
    SaveRegistry(src, outAr);

    ByteInputArchive inAr(outAr.Bytes());
    LoadRegistry(dest, inAr);

    CHECK(dest.valid(e1));
    CHECK(dest.valid(e2));
    CHECK(dest.valid(e3));
    CHECK_EQ(dest.get<Transform3D>(e1).Position, glm::vec3(0.0f, 2.0f, 1.5f));
    CHECK_EQ(dest.get<Health>(e2).Health, 300.0f);
    CHECK_EQ(dest.get<RigidBody>(e3).rigid_body, src.get<RigidBody>(e3).rigid_body);
}
