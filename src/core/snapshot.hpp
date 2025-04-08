#pragma once

#include <bx/bx.h>
#include <reactphysics3d/engine/PhysicsCommon.h>

#include <cstddef>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

#include "components/health.hpp"
#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "fmt/base.h"
#include "fmt/format.h"
#include "registry/registry.hpp"

enum DataType {
    Entity = 0,
    Transform,
    Size,
};

class ByteInputArchive
{
    using byte_stream = std::vector<std::byte>;

   public:
    ByteInputArchive(byte_stream& aInStream) : mStorage(aInStream), mIdx(0) {}

    void operator()(entt::entity& aEntity)
    {
        boundCheck(sizeof(aEntity));
        bx::memCopy(&aEntity, &mStorage[mIdx], sizeof(aEntity));
        mIdx += sizeof(aEntity);
        fmt::println("in entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
    }

    void operator()(std::underlying_type_t<entt::entity>& aEntity)
    {
        // DataType dt = readTypeHdr();

        boundCheck(sizeof(aEntity));
        bx::memCopy(&aEntity, &mStorage[mIdx], sizeof(aEntity));
        mIdx += sizeof(aEntity);

        fmt::println("in underlying type entity {:d}", aEntity);
    }

    void operator()(Transform3D& aTransform)
    {
        // readTypeHdr();
        boundCheck(10 * sizeof(float));

        bx::memCopy(&aTransform.Position, &mStorage[mIdx], 3 * sizeof(float));
        mIdx += 3 * sizeof(float);
        bx::memCopy(&aTransform.Orientation, &mStorage[mIdx], 4 * sizeof(float));
        mIdx += 4 * sizeof(float);
        bx::memCopy(&aTransform.Scale, &mStorage[mIdx], 3 * sizeof(float));
        mIdx += 3 * sizeof(float);
        fmt::println("in transform");
    }

    byte_stream Bytes() const { return mStorage; }

   private:
    void boundCheck(uint32_t aSize)
    {
        if (mIdx + aSize > mStorage.size()) {
            throw std::out_of_range(fmt::format(" mIdx = {:d}, asked = {:d}, size = {:d}",
                mIdx,
                aSize,
                mStorage.size()));
        }
    }
    DataType readTypeHdr()
    {
        DataType dType;
        boundCheck(sizeof(DataType));

        bx::memCopy(&dType, &mStorage[mIdx], sizeof(DataType));
        fmt::println("data type hdr: {:d}", static_cast<uint32_t>(dType));
        mIdx += sizeof(DataType);

        return dType;
    }

    byte_stream mStorage;
    uint32_t    mIdx;
};

class ByteOutputArchive
{
    using byte_stream = std::vector<std::byte>;

   public:
    ByteOutputArchive(byte_stream& aOutStream) : mStorage(aOutStream) {}

    void operator()(entt::entity aEntity)
    {
        fmt::println("out entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
        auto* p = reinterpret_cast<std::byte*>(&aEntity);
        mStorage.insert(mStorage.end(), p, p + sizeof(aEntity));
    }

    void operator()(std::underlying_type_t<entt::entity> aEntity)
    {
        fmt::println("out underlying type entity {:d}", aEntity);
        // writeTypeHdr(DataType::Size);
        auto* p = reinterpret_cast<std::byte*>(&aEntity);
        mStorage.insert(mStorage.end(), p, p + sizeof(aEntity));
    }

    void operator()(const Transform3D& aTransform)
    {
        fmt::println("out transform");
        // writeTypeHdr(DataType::Transform);
        auto* p = reinterpret_cast<const std::byte*>(glm::value_ptr(aTransform.Position));
        mStorage.insert(mStorage.end(), p, p + 3 * sizeof(float));

        p = reinterpret_cast<const std::byte*>(glm::value_ptr(aTransform.Orientation));
        mStorage.insert(mStorage.end(), p, p + 4 * sizeof(float));

        p = reinterpret_cast<const std::byte*>(glm::value_ptr(aTransform.Scale));
        mStorage.insert(mStorage.end(), p, p + 3 * sizeof(float));
    }

    byte_stream  Bytes() const { return mStorage; }
    byte_stream& Bytes() { return mStorage; }

   private:
    void writeTypeHdr(const DataType& aType)
    {
        auto  raw = static_cast<std::underlying_type_t<DataType>>(aType);
        auto* p   = reinterpret_cast<const std::byte*>(&raw);
        mStorage.insert(mStorage.end(), p, p + sizeof(DataType));
    }

    byte_stream mStorage;
};

template <typename OutArchive>
void SaveRegistry(const entt::registry& aRegistry, OutArchive& aArchive);

template <typename InArchive>
void LoadRegistry(entt::registry& aRegistry, InArchive& aArchive);

#include "doctest.h"
TEST_CASE("snapshot.simple")
{
    entt::registry      src;
    rp3d::PhysicsCommon comm;
    rp3d::PhysicsWorld* world = comm.createPhysicsWorld();

    auto e1 = src.create();
    src.emplace<Transform3D>(e1, glm::vec3(0.0f, 2.0f, 1.5f), glm::vec3(1.0f), glm::vec3(1.0f));

    auto e2 = src.create();
    src.emplace<Health>(e2, 300.0f);

    auto  e3 = src.create();
    auto* rb = world->createRigidBody(rp3d::Transform());

    src.emplace<RigidBody>(e3, rb);
    auto e4 = src.create();
    auto e5 = src.create();
    auto e6 = src.create();
    auto e7 = src.create();
    auto e8 = src.create();
    auto e9 = src.create();

    std::vector<std::byte> storage;
    ByteOutputArchive      outAr(storage);
    entt::registry         dest;
    SaveRegistry(src, outAr);

    CAPTURE(outAr.Bytes());
    CHECK_EQ(outAr.Bytes().size(), 32);

    ByteInputArchive inAr(outAr.Bytes());
    LoadRegistry(dest, inAr);

    CHECK(dest.valid(e1));
}
