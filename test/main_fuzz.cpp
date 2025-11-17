#include <spdlog/spdlog.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "core/serialize.hpp"
#include "core/snapshot.hpp"

template <typename T, typename MinMaxT>
struct ValueWithBounds {
    T       Value;
    MinMaxT Min;
    MinMaxT Max;

    void Clamp()
    {
        Value = std::clamp(Value, std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    };

    bool Valid() { return CheckBoundsAndVal(Value, Min, Max); }
};

template <typename T, typename M>
struct fmt::formatter<ValueWithBounds<T, M>> : fmt::formatter<std::string> {
    auto format(ValueWithBounds<T, M> const& aObj, format_context& aCtx) const
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
               && std::abs(F32V.Value - aO.F32V.Value) < 0.01f;
    }
};

template <>
struct fmt::formatter<TestData> : fmt::formatter<std::string> {
    auto format(TestData const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "I32: {}\nUI32: {}\nF32: {}\nBool: {}",
            aObj.I32V,
            aObj.UI32V,
            aObj.F32V,
            aObj.BV);
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

    // Lecture avec contraintes min/max
    template <typename T>
    std::optional<T> ReadClamped(T aMin, T aMax)
    {
        auto value = Read<T>();
        if (!value) return std::nullopt;
        return std::clamp(*value, aMin, aMax);
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

        // v.Clamp();
        // spdlog::info("clamped: {}", v);

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

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* aData, size_t aSize)
{
    if (aSize < sizeof(TestData)) {
        return 0;
    }

    FuzzDataReader fr(aData, aSize);

    TestData original;
    if (auto v = fr.ReadWithBounds<int32_t, int64_t>(); v)
        original.I32V = *v;
    else
        return 0;

    spdlog::info("got i32: {}", original.I32V);

    if (auto v = fr.ReadWithBounds<uint32_t, uint64_t>(); v)
        original.UI32V = *v;
    else
        return 0;
    spdlog::info("got ui32: {}", original.UI32V);

    if (auto v = fr.ReadWithBounds<float, float>(); v) {
        float range = v->Max - v->Min;
        if (range > 1e6f || range < 1e-6f) {
            return 0;
        }
        original.F32V = *v;
    } else
        return 0;
    spdlog::info("got f32: {}", original.F32V);
    spdlog::info("got test data:\n {}", original);

    if (auto v = fr.ReadBool(); v)
        original.BV = *v;
    else
        return 0;

    BitOutputArchive outArchive;
    if (!original.Archive(outArchive)) {
        return 0;
    }

    BitInputArchive inArchive(outArchive.Data());
    TestData        decoded = original;
    if (!decoded.Archive(inArchive)) {
        return 0;
    }

    BX_ASSERT(
        decoded == original,
        "FAIL:\nExpected:\n%s\n\nGot:\n%s",
        fmt::to_string(original).c_str(),
        fmt::to_string(decoded).c_str());

    return 0;
}
