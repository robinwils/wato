#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_relational.hpp>
#include <glm/gtc/epsilon.hpp>
#include <limits>

#include "core/serialize.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"

template <typename T, typename MinMaxT>
struct ValueWithBounds {
    T       Value;
    MinMaxT Min;
    MinMaxT Max;

    bool Valid() { return CheckBoundsAndVal(Value, Min, Max); }
};

template <glm::length_t L, typename T, typename MinMaxT, glm::qualifier Q = glm::defaultp>
struct GLMVecWithBounds {
    glm::vec<L, T, Q> Value;
    MinMaxT           Min;
    MinMaxT           Max;

    bool Valid() { return glm::all(CheckBoundsAndVal(Value, Min, Max)); }
};

template <typename T, typename M>
struct fmt::formatter<ValueWithBounds<T, M>> : fmt::formatter<std::string> {
    auto format(ValueWithBounds<T, M> const& aObj, format_context& aCtx) const
        -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{} [{} - {}]", aObj.Value, aObj.Min, aObj.Max);
    }
};

template <glm::length_t L, typename T, typename M, glm::qualifier Q>
struct fmt::formatter<GLMVecWithBounds<L, T, M, Q>> : fmt::formatter<std::string> {
    auto format(GLMVecWithBounds<L, T, M, Q> const& aObj, format_context& aCtx) const
        -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{} [{} - {}]", aObj.Value, aObj.Min, aObj.Max);
    }
};

struct TestData {
    ValueWithBounds<int32_t, int64_t>   I32V;
    ValueWithBounds<uint32_t, uint64_t> UI32V;
    ValueWithBounds<float, float>       F32V;
    bool                                BV;

    GLMVecWithBounds<3, float, float>           V1;
    GLMVecWithBounds<2, unsigned int, uint64_t> V2;
    GLMVecWithBounds<4, int, int64_t>           V3;

    bool Archive(auto& aR)
    {
        if (!ArchiveValue(aR, I32V.Value, I32V.Min, I32V.Max)) return false;
        if (!ArchiveValue(aR, UI32V.Value, UI32V.Min, UI32V.Max)) return false;
        if (!ArchiveValue(aR, F32V.Value, F32V.Min, F32V.Max)) return false;
        if (!ArchiveBool(aR, BV)) return false;
        return true;
    }

    bool operator==(const TestData& aO) const
    {
        return I32V.Value == aO.I32V.Value && UI32V.Value == aO.UI32V.Value && BV == aO.BV
               && std::abs(F32V.Value - aO.F32V.Value) < 0.01f
               && glm::all(glm::epsilonEqual(V1.Value, aO.V1.Value, 0.01f))
               && glm::all(glm::equal(V2.Value, aO.V2.Value))
               && glm::all(glm::equal(V3.Value, aO.V3.Value));
    }
};

struct AllTestData {
    TestData Data;

    std::optional<int32_t>  OptEmpty;
    std::optional<int32_t>  OptI32;
    std::optional<uint32_t> OptU32;
    std::optional<float>    OptF32;

    AllTestData(const TestData& aData)
        : Data(aData), OptI32(aData.I32V.Value), OptU32(aData.UI32V.Value), OptF32(aData.F32V.Value)
    {
    }

    bool Archive(auto& aR)
    {
        if (!Data.Archive(aR)) return false;
        if (!ArchiveOptionalVal(aR, OptI32, Data.I32V.Min, Data.I32V.Max)) return false;
        if (!ArchiveOptionalVal(aR, OptU32, Data.UI32V.Min, Data.UI32V.Max)) return false;
        if (!ArchiveOptionalVal(aR, OptF32, Data.F32V.Min, Data.F32V.Max)) return false;
        return true;
    }

    bool operator==(const AllTestData& aO) const
    {
        return Data == aO.Data && OptEmpty == aO.OptEmpty && OptI32 == aO.OptI32
               && OptU32 == aO.OptU32 && OptF32 == aO.OptF32;
    }
};

template <>
struct fmt::formatter<TestData> : fmt::formatter<std::string> {
    auto format(TestData const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "I32: {}\nUI32: {}\nF32: {}\nBool: {}\nVec3: {}\nUVec2: {}\nIVec4: {}",
            aObj.I32V,
            aObj.UI32V,
            aObj.F32V,
            aObj.BV,
            aObj.V1,
            aObj.V2,
            aObj.V3);
    }
};

template <>
struct fmt::formatter<AllTestData> : fmt::formatter<std::string> {
    auto format(AllTestData const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "{}\nOptEmpty: {}\nOptI32: {}\nOptUI32: {}\nOptF32: {}\n",
            aObj.Data,
            aObj.OptEmpty,
            aObj.OptI32,
            aObj.OptU32,
            aObj.OptF32);
    }
};

class FuzzDataReader
{
   public:
    FuzzDataReader(const uint8_t* aData, size_t aSize) : mData(aData), mSize(aSize), mOffset(0) {}

    // Lecture basique d'un type
    template <typename T>
    std::optional<T> Read()
    {
        if (mOffset + sizeof(T) > mSize) return std::nullopt;

        T value;
        std::memcpy(&value, mData + mOffset, sizeof(T));
        mOffset += sizeof(T);
        return value;
    }

    template <typename T, typename MinMaxT>
    std::optional<ValueWithBounds<T, MinMaxT>> ReadWithBounds()
    {
        auto value = Read<T>();
        auto min   = Read<MinMaxT>();
        auto max   = Read<MinMaxT>();

        if (!value || !min || !max || min == max) return std::nullopt;

        // Assurer min <= max
        if (*min > *max) std::swap(*min, *max);

        auto v = ValueWithBounds<T, MinMaxT>{*value, *min, *max};

        if (!v.Valid()) return std::nullopt;

        return v;
    }

    template <glm::length_t L, typename T, glm::qualifier Q, typename MinMaxT>
    std::optional<GLMVecWithBounds<L, T, MinMaxT, Q>> ReadGLMVec()
    {
        glm::vec<L, T, Q> vec;

        for (glm::length_t n = 0; n < L; ++n) {
            auto value = Read<T>();

            if (!value) return std::nullopt;

            vec[n] = *value;
        }

        auto min = Read<MinMaxT>();
        auto max = Read<MinMaxT>();

        auto v = GLMVecWithBounds{vec, *min, *max};

        if (!v.Valid()) return std::nullopt;

        return v;
    }

    // Lecture bool
    std::optional<bool> ReadBool()
    {
        auto byte = Read<uint8_t>();
        if (!byte) return std::nullopt;
        return (*byte & 1) == 1;
    }

    // Helpers
    size_t Remaining() const { return mSize - mOffset; }
    bool   HasBytes(size_t aN) const { return mOffset + aN <= mSize; }
    void   Reset() { mOffset = 0; }

   private:
    const uint8_t* mData;
    size_t         mSize;
    size_t         mOffset;
};

#define READ_GLM_VEC3(r)  (r.ReadGLMVec<3, float, glm::defaultp, float>())
#define READ_GLM_UVEC2(r) (r.ReadGLMVec<2, unsigned int, glm::defaultp, uint64_t>())
#define READ_GLM_IVEC4(r) (r.ReadGLMVec<4, int, glm::defaultp, int64_t>())

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* aData, size_t aSize)
{
    if (aSize < sizeof(TestData)) {
        return 0;
    }

    Logger logger = WATO_SER_LOGGER;
    logger->set_level(spdlog::level::off);

    FuzzDataReader fr(aData, aSize);

    TestData original;
    if (auto v = fr.ReadWithBounds<int32_t, int64_t>(); v)
        original.I32V = *v;
    else
        return 0;
    logger->info("got i32: {}", original.I32V);

    if (auto v = fr.ReadWithBounds<uint32_t, uint64_t>(); v)
        original.UI32V = *v;
    else
        return 0;
    logger->info("got ui32: {}", original.UI32V);

    if (auto v = fr.ReadWithBounds<float, float>(); v) {
        float range = v->Max - v->Min;
        if (range > 1e6f || range < 1e-6f) {
            return 0;
        }
        original.F32V = *v;
    } else
        return 0;
    logger->info("got f32: {}", original.F32V);
    logger->info("got test data:\n {}", original);

    if (auto v = fr.ReadBool(); v)
        original.BV = *v;
    else
        return 0;

    if (auto v = READ_GLM_VEC3(fr); v) {
        original.V1 = *v;
    } else
        return 0;
    if (auto v = READ_GLM_UVEC2(fr); v) {
        original.V2 = *v;
    } else
        return 0;
    if (auto v = READ_GLM_IVEC4(fr); v) {
        original.V3 = *v;
    } else
        return 0;

    AllTestData      data(original);
    BitOutputArchive outArchive;

    if (!data.Archive(outArchive)) {
        return 0;
    }

    BitInputArchive inArchive(outArchive.Data());
    AllTestData     decoded = data;
    if (!decoded.Archive(inArchive)) {
        return 0;
    }

    BX_ASSERT(
        decoded == original,
        "FAIL:\nExpected:\n%s\n\nGot:\n%s",
        fmt::to_string(data).c_str(),
        fmt::to_string(decoded).c_str());

    return 0;
}
