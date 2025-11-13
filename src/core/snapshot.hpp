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

class ByteInputArchive
{
   public:
    using byte        = uint8_t;
    using byte_stream = std::span<byte>;

    ByteInputArchive(
        const byte_stream& aInStream,
        bool               aEnableLogger = false,
        const Logger&      aLogger       = spdlog::default_logger())
        : mStorage(aInStream), mIdx(0), mLogger(aLogger)
    {
        if (!aEnableLogger) {
            mLogger->set_level(spdlog::level::off);
        }
    }
    ByteInputArchive(
        std::vector<byte> aInVec,
        bool              aEnableLogger = false,
        const Logger&     aLogger       = spdlog::default_logger())
        : mStorage(aInVec.begin(), aInVec.size()), mIdx(0), mLogger(aLogger)
    {
        if (!aEnableLogger) {
            mLogger->set_level(spdlog::level::off);
        }
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
    ByteOutputArchive(bool aEnableLogger = false, const Logger& aLogger = spdlog::default_logger())
        : mLogger(aLogger)
    {
        if (!aEnableLogger) {
            mLogger->set_level(spdlog::level::off);
        }
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

class BitInputArchive : public StreamDecoder
{
   public:
    BitInputArchive(
        BitReader::bit_stream&& aBits,
        bool                    aEnableLogger = false,
        const Logger&           aLogger       = spdlog::default_logger())
        : StreamDecoder(std::move(aBits)), mLogger(aLogger)
    {
        if (!aEnableLogger) {
            mLogger->set_level(spdlog::level::off);
        }
    }
    BitInputArchive(
        BitReader::bit_buffer& aBits,
        bool                   aEnableLogger = false,
        const Logger&          aLogger       = spdlog::default_logger())
        : StreamDecoder(aBits), mLogger(aLogger)
    {
        if (!aEnableLogger) {
            mLogger->set_level(spdlog::level::off);
        }
    }

    void operator()(entt::entity& aEntity)
    {
        ENTT_ID_TYPE entityV;
        if (!DecodeUInt(entityV, 0, std::numeric_limits<uint32_t>::max())) {
            mLogger->critical("could not decode entity");
            return;
        }

        aEntity = entt::entity(entityV);
        mLogger->info("read entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
    }

    void operator()(std::underlying_type_t<entt::entity>& aEntity)
    {
        if (!DecodeUInt(aEntity, 0, std::numeric_limits<uint32_t>::max())) {
            mLogger->critical("could not read component set size");
            return;
        }
        mLogger->info("=====> reading set of size {:d} <=====", aEntity);
    }

    template <typename T>
    void operator()(T& aObj)
    {
        if (!aObj.Archive(*this)) {
            mLogger->critical("could not read component from archive");
        }
    }

   private:
    Logger mLogger;
};

class BitOutputArchive : public StreamEncoder
{
   public:
    BitOutputArchive(bool aEnableLogger = false, const Logger& aLogger = spdlog::default_logger())
        : mLogger(aLogger)
    {
        if (!aEnableLogger) {
            mLogger->set_level(spdlog::level::off);
        }
    }

    void operator()(entt::entity aEntity)
    {
        ENTT_ID_TYPE entityV = static_cast<ENTT_ID_TYPE>(aEntity);
        mLogger->info("writing entity {:d}", static_cast<ENTT_ID_TYPE>(aEntity));
        EncodeUInt(entityV, 0, std::numeric_limits<uint32_t>::max());
    }

    void operator()(std::underlying_type_t<entt::entity> aEntity)
    {
        mLogger->info("<===== writing set of size {:d} =====> ", aEntity);
        EncodeUInt(aEntity, 0, std::numeric_limits<uint32_t>::max());
    }

    template <typename T>
    void operator()(const T& aObj)
    {
        // const casting here because we are calling generic Archive methods that will
        // either call Encode or Decode depending on the archive.
        // EnTT uses a const registry for snapshot so we can't bypass const otherwise,
        // and this avoids duplicating Archive calls on every component type
        const_cast<T&>(aObj).Archive(*this);
    }

    BitWriter::bit_buffer&     Data() { return mBits.Data(); }
    const std::span<uint8_t>   Bytes() { return mBits.Bytes(); }
    const std::vector<uint8_t> ByteVector() { return mBits.ByteVector(); }

   private:
    Logger mLogger;
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

#ifndef DOCTEST_CONFIG_DISABLE
#include "test.hpp"

TEST_CASE("serialize.scalar")
{
    BitOutputArchive outAr;

    int32_t i = 123456;
    float   f = 3.14159f;
    ArchiveValue(outAr, i, 0, 666666);
    ArchiveValue(outAr, f, 0.0f, 100.0f);

    BitInputArchive inAr(outAr.Data());
    int32_t         i2 = 0;
    float           f2 = 0.0f;
    ArchiveValue(inAr, i2, 0, 666666);
    ArchiveValue(inAr, f2, 0.0f, 100.0f);

    CHECK_EQ(i2, i);
    CHECK_EQ(f2, doctest::Approx(f).epsilon(0.0001));
}

TEST_CASE("serialize.vector_of_scalars")
{
    BitOutputArchive outAr;

    std::vector<uint16_t> v = {10, 20, 30, 40, 50};
    ArchiveVector(outAr, v, 0u, 100u, 32);

    BitInputArchive       inAr(outAr.Data());
    std::vector<uint16_t> v2;
    ArchiveVector(inAr, v2, 0u, 100u, 32);

    CHECK_EQ(v2, v);
}

TEST_CASE("serialize.enum")
{
    BitOutputArchive outAr;

    TestEnum e = TestEnum::B;
    ArchiveValue(outAr, e, 0u, uint32_t(TestEnum::Count));

    BitInputArchive inAr(outAr.Data());
    TestEnum        e2 = TestEnum::A;
    ArchiveValue(inAr, e2, 0u, uint32_t(TestEnum::Count));

    CHECK(e2 == e);
}

TEST_CASE("serialize.vector_of_enum")
{
    BitOutputArchive outAr;

    std::vector<TestEnum> v = {TestEnum::A, TestEnum::C, TestEnum::B};
    ArchiveVector(outAr, v, 0u, uint32_t(TestEnum::Count), 32);

    BitInputArchive       inAr(outAr.Data());
    std::vector<TestEnum> v2;
    ArchiveVector(outAr, v2, 0u, uint32_t(TestEnum::Count), 32);

    CHECK_EQ(v2, v);
}

TEST_CASE("serialize.rigidbody")
{
    BitOutputArchive outAr(true);

    RigidBody rb;
    rb.Params.Type           = rp3d::BodyType::DYNAMIC;
    rb.Params.Velocity       = 42.0f;
    rb.Params.Direction      = glm::vec3(0.5f, 0.5f, 0.5f);
    rb.Params.GravityEnabled = true;

    rb.Archive(outAr);

    BitInputArchive inAr(outAr.Data(), true);
    RigidBody       rb2;
    rb2.Archive(inAr);

    CHECK_EQ(rb2.Params.Type, rb.Params.Type);
    CHECK_EQ(rb2.Params.Velocity, doctest::Approx(rb.Params.Velocity));
    CHECK_VEC3_APPROX_EPSILON(rb2.Params.Direction, rb.Params.Direction, 0.0001);
    CHECK_EQ(rb2.Params.GravityEnabled, rb.Params.GravityEnabled);
}

TEST_CASE("serialize.collider.box")
{
    BitOutputArchive outAr(true);

    Collider c;
    c.Params.CollisionCategoryBits = Category::Entities;
    c.Params.CollideWithMaskBits   = Category::Terrain;
    c.Params.IsTrigger             = false;
    c.Params.Offset.Position       = glm::vec3(1.0f, 2.0f, 3.0f);
    c.Params.Offset.Orientation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    BoxShapeParams box;
    box.HalfExtents      = glm::vec3(0.5f, 0.5f, 0.5f);
    c.Params.ShapeParams = box;

    c.Archive(outAr);

    BitInputArchive inAr(outAr.Data(), true);
    Collider        c2;
    c2.Archive(inAr);

    CHECK_EQ(c2.Params.CollisionCategoryBits, c.Params.CollisionCategoryBits);
    CHECK_EQ(c2.Params.CollideWithMaskBits, c.Params.CollideWithMaskBits);
    CHECK_EQ(c2.Params.IsTrigger, c.Params.IsTrigger);
    CHECK_VEC3_APPROX_EPSILON(c2.Params.Offset.Position, c.Params.Offset.Position, 0.0001);
    // Rotation equality may need to use Approx or custom check
    CHECK(glm::all(glm::epsilonEqual(
        glm::quat(c2.Params.Offset.Orientation),
        glm::quat(c.Params.Offset.Orientation),
        0.0001f)));
    CHECK(std::holds_alternative<BoxShapeParams>(c2.Params.ShapeParams));
    if (std::holds_alternative<BoxShapeParams>(c2.Params.ShapeParams)) {
        auto& b1 = std::get<BoxShapeParams>(c.Params.ShapeParams);
        auto& b2 = std::get<BoxShapeParams>(c2.Params.ShapeParams);
        CHECK_VEC3_APPROX_EPSILON(b2.HalfExtents, b1.HalfExtents, 0.0001);
    }
}

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
    src.emplace<Health>(e2, 100.0f);

    BitOutputArchive outAr(true);
    entt::registry   dest;
    entt::snapshot{src}.get<entt::entity>(outAr).get<Transform3D>(outAr).get<Health>(outAr);

    BitInputArchive inAr(outAr.Data(), true);
    entt::snapshot_loader{dest}.get<entt::entity>(inAr).get<Transform3D>(inAr).get<Health>(inAr);

    CHECK(dest.valid(e1));
    CHECK(dest.valid(e2));
    auto& t1 = dest.get<Transform3D>(e1);
    auto& h2 = dest.get<Health>(e2);
    CHECK_VEC3_APPROX_EPSILON_VALUES(t1.Position, 0.0, 2.0, 1.5, 0.0001);
    CHECK_EQ(h2.Health, 100.0f);
}

TEST_CASE("snapshot.full")
{
    entt::registry src;

    auto            e1 = src.create();
    RigidBodyParams rbParams;
    rbParams.Type           = rp3d::BodyType::DYNAMIC;
    rbParams.GravityEnabled = true;
    rbParams.Velocity       = 42.0f;
    rbParams.Direction      = glm::vec3(0.0f, 1.0f, 0.0f);
    src.emplace<RigidBody>(e1, RigidBody{rbParams, nullptr});
    ColliderParams boxParams;
    boxParams.CollisionCategoryBits = Category::Entities | Category::Terrain;
    boxParams.CollideWithMaskBits   = Category::Entities;
    boxParams.IsTrigger             = false;
    boxParams.Offset.Position       = glm::vec3(1.0f, 2.0f, 3.0f);
    boxParams.Offset.Orientation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    boxParams.Offset.Scale          = glm::vec3(1.0f);
    BoxShapeParams boxShape;
    boxShape.HalfExtents  = glm::vec3(0.5f, 0.5f, 0.5f);
    boxParams.ShapeParams = boxShape;
    src.emplace<Collider>(e1, Collider{boxParams, nullptr});

    auto           e2 = src.create();
    ColliderParams capsuleParams;
    capsuleParams.CollisionCategoryBits = Category::Terrain;
    capsuleParams.CollideWithMaskBits   = Category::Entities;
    capsuleParams.IsTrigger             = true;
    capsuleParams.Offset.Position       = glm::vec3(2.0f, 3.0f, 4.0f);
    capsuleParams.Offset.Orientation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    capsuleParams.Offset.Scale          = glm::vec3(2.0f);
    CapsuleShapeParams capsuleShape;
    capsuleShape.Radius       = 1.5f;
    capsuleShape.Height       = 3.5f;
    capsuleParams.ShapeParams = capsuleShape;
    src.emplace<Collider>(e2, Collider{capsuleParams, nullptr});

    auto           e3 = src.create();
    ColliderParams heightfieldParams;
    heightfieldParams.CollisionCategoryBits = Category::Terrain;
    heightfieldParams.CollideWithMaskBits   = Category::Terrain | Category::Entities;
    heightfieldParams.IsTrigger             = false;
    heightfieldParams.Offset.Position       = glm::vec3(3.0f, 4.0f, 5.0f);
    heightfieldParams.Offset.Orientation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    heightfieldParams.Offset.Scale          = glm::vec3(3.0f);
    HeightFieldShapeParams heightfieldShape;
    heightfieldShape.Data         = {1.0f, 2.0f, 3.0f, 4.0f};
    heightfieldShape.Rows         = 2;
    heightfieldShape.Columns      = 2;
    heightfieldParams.ShapeParams = heightfieldShape;
    src.emplace<Collider>(e3, Collider{heightfieldParams, nullptr});

    BitOutputArchive outAr(true);
    entt::registry   dest;
    SaveRegistry(src, outAr);

    BitInputArchive inAr(outAr.Data(), true);
    entt::snapshot_loader{dest}
        .get<entt::entity>(inAr)
        .get<Transform3D>(inAr)
        .get<Health>(inAr)
        .get<RigidBody>(inAr)
        .get<Collider>(inAr);

    CHECK(dest.valid(e1));
    CHECK(dest.valid(e2));
    CHECK(dest.valid(e3));

    const auto& rb = dest.get<RigidBody>(e1);
    CHECK_EQ(rb.Params.Type, rbParams.Type);
    CHECK_EQ(rb.Params.GravityEnabled, rbParams.GravityEnabled);
    CHECK_EQ(rb.Params.Velocity, doctest::Approx(rbParams.Velocity));
    CHECK_VEC3_APPROX_EPSILON(rb.Params.Direction, rbParams.Direction, 0.0001);

    const auto& colBox = dest.get<Collider>(e1);
    CHECK_EQ(colBox.Params.CollisionCategoryBits, boxParams.CollisionCategoryBits);
    CHECK_EQ(colBox.Params.CollideWithMaskBits, boxParams.CollideWithMaskBits);
    CHECK_EQ(colBox.Params.IsTrigger, boxParams.IsTrigger);
    CHECK_VEC3_APPROX_EPSILON(colBox.Params.Offset.Position, boxParams.Offset.Position, 0.0001);
    CHECK(std::holds_alternative<BoxShapeParams>(colBox.Params.ShapeParams));
    const auto& box = std::get<BoxShapeParams>(colBox.Params.ShapeParams);
    CHECK_VEC3_APPROX_EPSILON_VALUES(box.HalfExtents, 0.5, 0.5, 0.5, 0.0001);

    const auto& colCapsule = dest.get<Collider>(e2);
    CHECK_EQ(colCapsule.Params.CollisionCategoryBits, Category::Terrain);
    CHECK_EQ(colCapsule.Params.CollideWithMaskBits, Category::Entities);
    CHECK(colCapsule.Params.IsTrigger == true);
    CHECK_VEC3_APPROX_EPSILON(
        colCapsule.Params.Offset.Position,
        capsuleParams.Offset.Position,
        0.0001);
    CHECK(std::holds_alternative<CapsuleShapeParams>(colCapsule.Params.ShapeParams));
    const auto& capsule = std::get<CapsuleShapeParams>(colCapsule.Params.ShapeParams);
    CHECK(capsule.Radius == doctest::Approx(1.5).epsilon(0.0001));
    CHECK(capsule.Height == doctest::Approx(3.5));

    const auto& colHeightfield = dest.get<Collider>(e3);
    CHECK_EQ(colHeightfield.Params.CollisionCategoryBits, Category::Terrain);
    CHECK_EQ(colHeightfield.Params.CollideWithMaskBits, Category::Terrain | Category::Entities);
    CHECK(colHeightfield.Params.IsTrigger == false);
    CHECK_VEC3_APPROX_EPSILON_VALUES(colHeightfield.Params.Offset.Position, 3.0, 4.0, 5.0, 0.0001);
    CHECK(std::holds_alternative<HeightFieldShapeParams>(colHeightfield.Params.ShapeParams));
    const auto& heightfield = std::get<HeightFieldShapeParams>(colHeightfield.Params.ShapeParams);
    // CHECK(heightfield.Data == std::vector<float>({1.0f, 2.0f, 3.0f, 4.0f}));
    CHECK(heightfield.Rows == 2);
    CHECK(heightfield.Columns == 2);
}
#endif
