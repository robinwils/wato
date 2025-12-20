#pragma once

#include <bx/bx.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <entt/entity/entity.hpp>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vector_relational.hpp>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <variant>
#include <vector>

#include "core/sys/log.hpp"
#include "core/types.hpp"

#if defined(_MSC_VER)
#include <intrin.h>

inline std::uint16_t byteswap(std::uint16_t aX) { return _byteswap_ushort(aX); }
inline std::uint32_t byteswap(std::uint32_t aX) { return _byteswap_ulong(aX); }
inline std::uint64_t byteswap(std::uint64_t aX) { return _byteswap_uint64(aX); }
#elif defined(__clang__) || defined(__GNUC__)
constexpr inline std::uint16_t byteswap(std::uint16_t aX) { return __builtin_bswap16(aX); }
constexpr inline std::uint32_t byteswap(std::uint32_t aX) { return __builtin_bswap32(aX); }
constexpr inline std::uint64_t byteswap(std::uint64_t aX) { return __builtin_bswap64(aX); }
#endif

template <class T>
constexpr T bswap_any(T v)
{
    if constexpr (sizeof(T) == 1) {
        return v;
    } else if constexpr (sizeof(T) == 2) {
        auto u = std::bit_cast<std::uint16_t>(v);
        return std::bit_cast<T>(byteswap(u));
    } else if constexpr (sizeof(T) == 4) {
        auto u = std::bit_cast<std::uint32_t>(v);
        return std::bit_cast<T>(byteswap(u));
    } else if constexpr (sizeof(T) == 8) {
        auto u = std::bit_cast<std::uint64_t>(v);
        return std::bit_cast<T>(byteswap(u));
    } else {
        static_assert(sizeof(T) <= 8, "Unsupported size");
        return v;
    }
}

using word       = uint32_t;
using bit_stream = std::span<word>;
using bit_buffer = std::vector<word>;

class BitWriter
{
   public:
    BitWriter() : mScratch(0), mCurBit(0) {}
    BitWriter(const BitWriter& aBuf) : mBuf(aBuf.mBuf), mScratch(0), mCurBit(0) {}

    void Write(uint64_t aData, uint32_t aN)
    {
        uint32_t lo = uint32_t(aData & 0xffffffff);
        uint32_t hi = uint32_t(aData >> 32);

        if (aN <= 32) {
            Write(lo, aN);
        } else {
            Write(lo, 32);
            Write(hi, aN - 32);
        }
    }

    void Write(uint32_t aData, uint32_t aN)
    {
        BX_ASSERT(aN <= 32, "cannot write more than 32 bits");

        // mask input data to be exactly aN bits
        aData    &= (uint64_t(1) << aN) - 1lu;
        mScratch |= (SafeU64(aData) << mCurBit);

        mCurBit += aN;

        if (mCurBit >= 32) {
            if constexpr (std::endian::native == std::endian::little) {
                mBuf.push_back(SafeU32(mScratch & 0xffffffff));
            } else {
                mBuf.push_back(bswap_any(SafeU32(mScratch & 0xffffffff)));
            }

            mScratch >>= 32;
            mCurBit   -= 32;
        }
    }

    bit_buffer& Data()
    {
        flush();
        return mBuf;
    }

    const std::span<uint8_t> Bytes()
    {
        uint8_t* byteView = std::bit_cast<uint8_t*>(Data().data());

        return std::span<uint8_t>(byteView, Data().size() * sizeof(word));
    }

    const std::vector<uint8_t> ByteVector()
    {
        uint8_t*           byteView = std::bit_cast<uint8_t*>(Data().data());
        std::size_t        s        = Data().size() * sizeof(word);
        std::span<uint8_t> v(byteView, s);

        return std::vector<uint8_t>(v.begin(), v.end());
    }

   private:
    void flush()
    {
        if (mCurBit > 0) {
            if constexpr (std::endian::native == std::endian::little) {
                mBuf.push_back(SafeU32(mScratch & 0xffffffff));
            } else {
                mBuf.push_back(bswap_any(SafeU32(mScratch & 0xffffffff)));
            }

            mScratch >>= 32;
            mCurBit    = 0;
        }
    }

    bit_buffer mBuf;
    uint64_t   mScratch;
    uint32_t   mCurBit;
};

class BitReader
{
   public:
    BitReader() : mScratch(0), mCurBit(0) {}
    BitReader(bit_stream&& aBits)
        : mBuf(std::move(aBits)), mScratch(0), mCurBit(0), mNext(mBuf.data())
    {
    }
    BitReader(bit_buffer& aBits)
        : mBuf(std::span(aBits)), mScratch(0), mCurBit(0), mNext(mBuf.data())
    {
    }

    bool Read(uint64_t& aData, uint32_t aN)
    {
        if (aN <= 32) {
            uint32_t lo = 0;
            if (!Read(lo, aN)) {
                return false;
            }
            aData = lo;
        } else {
            uint32_t lo = 0;
            uint32_t hi = 0;
            if (!Read(lo, 32)) {
                return false;
            }
            if (!Read(hi, aN - 32)) {
                return false;
            }
            aData = (uint64_t(hi) << 32) | lo;
        }
        return true;
    }

    bool Read(uint32_t& aData, uint32_t aN)
    {
        BX_ASSERT(aN <= 32, "cannot write more than 32 bits");

        // not enough bits in scratch, read next word
        if (aN > mCurBit) {
            if (!mNext || mNext >= mBuf.data() + mBuf.size()) {
                return false;
            }
            uint32_t next = *mNext++;

            if constexpr (std::endian::native == std::endian::little) {
                mScratch |= SafeU64(next) << mCurBit;
            } else {
                mScratch |= SafeU64(bswap_any(next)) << mCurBit;
            }
            mCurBit += 32;
        }

        aData      = SafeU32(mScratch & ((uint64_t(1) << aN) - 1));
        mScratch >>= aN;
        mCurBit   -= aN;

        return true;
    }

   private:
    bit_stream mBuf;
    uint64_t   mScratch;
    uint32_t   mCurBit;
    word*      mNext;
};

template <typename T, typename M>
void AssertBounds(M aMin, M aMax)
{
    M typeMin;
    if constexpr (std::is_floating_point_v<M>) {
        BX_ASSERT(std::isfinite(aMin), "%s", fmt::to_string(aMin).c_str());
        BX_ASSERT(std::isfinite(aMax), "%s", fmt::to_string(aMax).c_str());
        typeMin = -std::numeric_limits<T>::max();
    } else {
        typeMin = std::numeric_limits<T>::min();
    }

    BX_ASSERT(aMin < aMax, "%s >= %s", fmt::to_string(aMin).c_str(), fmt::to_string(aMax).c_str());

    if constexpr (sizeof(T) <= sizeof(int32_t)) {
        BX_ASSERT(
            aMax <= std::numeric_limits<T>::max(),
            "%s > %s",
            fmt::to_string(aMax).c_str(),
            fmt::to_string(std::numeric_limits<T>::max()).c_str());

        BX_ASSERT(
            aMin >= typeMin,
            "%s < %s",
            fmt::to_string(aMin).c_str(),
            fmt::to_string(typeMin).c_str());
    }
}

template <typename T, typename M>
void AssertBoundsAndVal(T aVal, M aMin, M aMax)
{
    AssertBounds<T, M>(aMin, aMax);

    if constexpr (std::is_floating_point_v<M>) {
        BX_ASSERT(std::isfinite(aVal), "%s", fmt::to_string(aVal).c_str());
    }
    BX_ASSERT(aVal >= aMin, "%s < %s", fmt::to_string(aVal).c_str(), fmt::to_string(aMin).c_str());
    BX_ASSERT(aVal <= aMax, "%s > %s", fmt::to_string(aVal).c_str(), fmt::to_string(aMax).c_str());
}

template <typename T, typename M>
bool CheckBounds(M aMin, M aMax)
{
    Logger logger = WATO_SER_LOGGER;

    if constexpr (std::is_floating_point_v<M>) {
        if (!std::isfinite(aMin)) {
            logger->critical("min {} is not finite", aMin);
            return false;
        }
        if (!std::isfinite(aMax)) {
            logger->critical("max {} is not finite", aMax);
            return false;
        }
    }
    if (aMin >= aMax) {
        logger->critical("min >= max: {} >= {}", aMin, aMax);
        return false;
    }

    if constexpr (sizeof(T) <= sizeof(int32_t)) {
        if (aMax > std::numeric_limits<T>::max()) {
            logger->critical("max > lim: {} > {}", aMax, std::numeric_limits<T>::max());
            return false;
        }
        if (aMin < std::numeric_limits<T>::min()) {
            logger->critical("min < lim: {} < {}", aMin, std::numeric_limits<T>::min());
            return false;
        }
    }
    return true;
}

template <typename T, typename M>
bool CheckBoundsAndVal(T aVal, M aMin, M aMax)
{
    Logger logger = WATO_SER_LOGGER;

    if (!CheckBounds<T, M>(aMin, aMax)) return false;

    if constexpr (std::is_floating_point_v<M>) {
        if (!std::isfinite(aVal)) {
            logger->critical("val {} is not finite", aVal);
            return false;
        }
    }

    if (aVal < aMin) {
        logger->critical("val < min: {} < {}", aVal, aMin);
        return false;
    }

    if (aVal > aMax) {
        logger->critical("val > max: {} > {}", aVal, aMax);
        return false;
    }

    return true;
}

template <glm::length_t L, typename T, glm::qualifier Q, typename M>
glm::vec<L, bool, Q> CheckBoundsAndVal(glm::vec<L, T, Q> aVal, M aMin, M aMax)
{
    glm::vec<L, bool, Q> res(true);
    for (glm::length_t idx = 0; idx < L; ++idx) {
        res[idx] = CheckBoundsAndVal(aVal[idx], aMin, aMax);
    }
    return res;
}

class StreamEncoder
{
   public:
    StreamEncoder() = default;

    void EncodeBool(const bool& aVal) { mBits.Write(aVal ? 1u : 0u, 1); }

    template <typename IntT>
        requires(
            std::is_integral_v<IntT> && !std::is_unsigned_v<IntT>
            && sizeof(IntT) <= sizeof(int32_t))
    void EncodeInt(IntT aVal, int64_t aMin, int64_t aMax)
    {
        AssertBoundsAndVal(aVal, aMin, aMax);

        uint64_t diff = aMax - aMin;

        mBits.Write(uint32_t(aVal - aMin), uint32_t(std::bit_width(diff)));
    }

    template <typename UIntT>
        requires(std::is_integral_v<UIntT> && std::is_unsigned_v<UIntT>)
    void EncodeUInt(UIntT aVal, uint64_t aMin, uint64_t aMax)
    {
        AssertBoundsAndVal(aVal, aMin, aMax);

        if constexpr (sizeof(UIntT) <= sizeof(uint32_t)) {
            mBits.Write(uint32_t(aVal - aMin), uint32_t(std::bit_width(aMax - aMin)));
        } else {
            mBits.Write(uint64_t(aVal - aMin), uint32_t(std::bit_width(aMax - aMin)));
        }
    }

    void EncodeFloat(float aVal)
    {
        uint32_t bits = std::bit_cast<uint32_t>(aVal);
        mBits.Write(bits, 32);
    }

    bit_buffer& Data() { return mBits.Data(); }

   protected:
    BitWriter mBits;
};

class StreamDecoder
{
   public:
    StreamDecoder() = default;
    StreamDecoder(bit_stream&& aBits) : mBits(std::move(aBits)) {}
    StreamDecoder(bit_buffer& aBits) : mBits(aBits) {}
    StreamDecoder(uint8_t* aBytes, std::size_t aSize)
        : mBits(bit_stream(std::bit_cast<word*>(aBytes), aSize))
    {
    }

    bool DecodeBool(bool& aVal)
    {
        uint32_t v = 0;
        if (!mBits.Read(v, 1)) {
            return false;
        }

        aVal = v == 1u;
        return true;
    }

    template <typename IntT>
        requires(std::is_integral_v<IntT> && !std::is_unsigned_v<IntT>)
    bool DecodeInt(IntT& aVal, int64_t aMin, int64_t aMax)
    {
        AssertBounds<IntT>(aMin, aMax);

        if constexpr (sizeof(IntT) <= sizeof(int32_t)) {
            uint32_t v = 0;
            if (!mBits.Read(v, std::bit_width(uint64_t(aMax - aMin)))) {
                return false;
            }
            int64_t vv = v + aMin;

            aVal = IntT(vv);
        } else {
            uint64_t v = 0;
            if (!mBits.Read(v, std::bit_width(uint64_t(aMax - aMin)))) {
                return false;
            }
            aVal = IntT(v) + aMin;
        }
        return true;
    }

    template <typename UIntT>
        requires(std::is_integral_v<UIntT> && std::is_unsigned_v<UIntT>)
    bool DecodeUInt(UIntT& aVal, uint64_t aMin, uint64_t aMax)
    {
        AssertBounds<UIntT>(aMin, aMax);

        if constexpr (sizeof(UIntT) <= sizeof(uint32_t)) {
            uint32_t v;
            if (!mBits.Read(v, uint32_t(std::bit_width(aMax - aMin)))) {
                return false;
            }
            aVal = UIntT(v) + UIntT(aMin);
        } else {
            uint64_t v;
            if (!mBits.Read(v, uint32_t(std::bit_width(aMax - aMin)))) {
                return false;
            }
            aVal = v + aMin;
        }
        return true;
    }

    bool DecodeFloat(float& aVal)
    {
        uint32_t bits;
        if (!mBits.Read(bits, 32)) return 0.0f;
        aVal = std::bit_cast<float>(bits);
        return true;
    }

   protected:
    BitReader mBits;
};

template <typename T>
concept IsStreamEncoder = requires(T t) {
    { t.EncodeBool(std::declval<const bool&>()) };
    { t.EncodeInt(std::declval<int>(), std::declval<int64_t>(), std::declval<int64_t>()) };
    { t.EncodeUInt(std::declval<uint8_t>(), std::declval<uint64_t>(), std::declval<uint64_t>()) };
    { t.EncodeUInt(std::declval<uint16_t>(), std::declval<uint64_t>(), std::declval<uint64_t>()) };
    { t.EncodeUInt(std::declval<uint64_t>(), std::declval<uint64_t>(), std::declval<uint64_t>()) };
};

template <typename T>
concept IsStreamDecoder = requires(
    T         t,
    int32_t&  vi,
    uint8_t&  vu8,
    uint16_t& vu16,
    uint32_t& vu,
    uint32_t& vu64,
    float&    vf) {
    { t.DecodeInt(vi, std::declval<int64_t>(), std::declval<int64_t>()) } -> std::same_as<bool>;
    { t.DecodeUInt(vu8, std::declval<uint64_t>(), std::declval<uint64_t>()) } -> std::same_as<bool>;
    {
        t.DecodeUInt(vu16, std::declval<uint64_t>(), std::declval<uint64_t>())
    } -> std::same_as<bool>;
    { t.DecodeUInt(vu, std::declval<uint64_t>(), std::declval<uint64_t>()) } -> std::same_as<bool>;
    { t.DecodeUInt(vu, std::declval<uint64_t>(), std::declval<uint64_t>()) } -> std::same_as<bool>;
};

template <typename T>
concept IsTriviallyArchivable = std::is_integral_v<std::remove_reference_t<T>>
                                || std::is_floating_point_v<std::remove_reference_t<T>>
                                || std::is_enum_v<std::remove_reference_t<T>>;

template <typename T, class Out>
concept IsArchivable = requires(T aObj, Out& aOut) {
    { aObj.Archive(aOut) } -> std::same_as<bool>;
};

template <typename Archive>
    requires(IsStreamEncoder<Archive> || IsStreamDecoder<Archive>)
bool ArchiveBool(Archive& aR, bool& aValue)
{
    if constexpr (IsStreamEncoder<Archive>) {
        aR.EncodeBool(aValue);
        return true;
    } else if constexpr (IsStreamDecoder<Archive>) {
        return aR.DecodeBool(aValue);
    }
    return false;
}

template <typename Archive, typename T, typename MinMaxT>
    requires(IsStreamEncoder<Archive> || IsStreamDecoder<Archive>) && IsTriviallyArchivable<T>
bool ArchiveValue(Archive& aR, T&& aValue, MinMaxT aMin, MinMaxT aMax)
{
    using value_t = std::remove_cvref_t<T>;

    if constexpr (std::is_enum_v<value_t>) {
        using U = std::underlying_type_t<value_t>;
        if constexpr (IsStreamEncoder<Archive>) {
            U val = static_cast<U>(aValue);
            WATO_SER_LOGGER->info("encoding enum {}", val);
            return ArchiveValue(aR, val, static_cast<U>(aMin), static_cast<U>(aMax));
        } else {
            U tmp;
            if (!ArchiveValue(aR, tmp, static_cast<U>(aMin), static_cast<U>(aMax))) return false;
            aValue = static_cast<value_t>(tmp);
            WATO_SER_LOGGER->info("decoded enum {}", tmp);
            return true;
        }
    } else if constexpr (std::is_same_v<value_t, float>) {
        if constexpr (IsStreamEncoder<Archive>) {
            WATO_SER_LOGGER->info("encoding float {}", aValue);
            aR.EncodeFloat(aValue);
            return true;
        } else {
            if (!aR.DecodeFloat(aValue)) return false;
            if (aValue > aMax || aValue < aMin) return false;
            WATO_SER_LOGGER->info("decoded float {}", aValue);
            return true;
        }
    } else if constexpr (std::is_integral_v<value_t> && std::is_signed_v<value_t>) {
        if constexpr (IsStreamEncoder<Archive>) {
            WATO_SER_LOGGER->info("encoding int {}", aValue);
            aR.EncodeInt(aValue, aMin, aMax);
            return true;
        } else {
            if (!aR.DecodeInt(aValue, aMin, aMax)) return false;
            if (aValue > aMax || aValue < aMin) return false;
            WATO_SER_LOGGER->info("decoded int {}", aValue);
            return true;
        }
    } else if constexpr (std::is_integral_v<value_t> && std::is_unsigned_v<value_t>) {
        if constexpr (IsStreamEncoder<Archive>) {
            WATO_SER_LOGGER->info("encoding uint{}_t {}", sizeof(value_t), aValue);
            aR.EncodeUInt(aValue, aMin, aMax);
            return true;
        } else {
            if (!aR.DecodeUInt(aValue, aMin, aMax)) return false;
            if (aValue > aMax || aValue < aMin) return false;
            WATO_SER_LOGGER->info("decoded uint{}_t {}", sizeof(value_t), aValue);
            return true;
        }
    }
    return false;
}

template <typename Archive>
    requires(IsStreamEncoder<Archive> || IsStreamDecoder<Archive>)
bool ArchiveEntity(Archive& aR, entt::entity& aValue)
{
    return ArchiveValue(aR, aValue, entt::entity{0}, entt::entity{entt::null});
}

template <typename Archive, typename T>
    requires(IsStreamEncoder<Archive> || IsStreamDecoder<Archive>)
bool ArchiveVector(Archive& aR, std::vector<T>& aVec, std::size_t aMaxSize)
{
    if constexpr (IsStreamEncoder<Archive>) {
        ArchiveValue(aR, aVec.size(), std::size_t(0), aMaxSize);
        for (auto& elt : aVec) {
            elt.Archive(aR);
        }
        return true;
    } else if constexpr (IsStreamDecoder<Archive>) {
        std::size_t s = 0;
        ArchiveValue(aR, s, std::size_t(0), aMaxSize);
        aVec.reserve(s);
        for (std::size_t idx = 0; idx < s; ++idx) {
            T elt;
            if (!elt.Archive(aR)) return false;
            aVec.push_back(std::move(elt));
        }
        return true;
    }
    return false;
}

template <typename Archive, typename T, typename MinMaxT>
    requires(IsStreamEncoder<Archive> || IsStreamDecoder<Archive>) && IsTriviallyArchivable<T>
bool ArchiveVector(
    Archive&        aR,
    std::vector<T>& aVec,
    MinMaxT         aMin,
    MinMaxT         aMax,
    std::size_t     aMaxSize)
{
    if constexpr (IsStreamEncoder<Archive>) {
        ArchiveValue(aR, aVec.size(), std::size_t(0), aMaxSize);
        for (auto& elt : aVec) {
            ArchiveValue(aR, elt, aMin, aMax);
        }
        return true;
    } else if constexpr (IsStreamDecoder<Archive>) {
        std::size_t s = 0;
        ArchiveValue(aR, s, std::size_t(0), aMaxSize);
        aVec.reserve(s);
        for (std::size_t idx = 0; idx < s; ++idx) {
            T elt{};
            if (!ArchiveValue(aR, elt, aMin, aMax)) return false;
            aVec.push_back(std::move(elt));
        }
        return true;
    }
    return false;
}

template <typename Archive, typename T, typename MinMaxT>
    requires(IsStreamEncoder<Archive> || IsStreamDecoder<Archive>) && IsTriviallyArchivable<T>
bool ArchiveOptionalVal(Archive& aR, std::optional<T>& aOpt, MinMaxT aMin, MinMaxT aMax)
{
    if constexpr (IsStreamEncoder<Archive>) {
        bool hasVal = aOpt.has_value();
        if (!ArchiveBool(aR, hasVal)) return false;
        if (aOpt) {
            auto val = *aOpt;
            if (!ArchiveValue(aR, val, aMin, aMax)) return false;
        }
        return true;
    } else if constexpr (IsStreamDecoder<Archive>) {
        bool hasVal;
        if (!ArchiveBool(aR, hasVal)) return false;
        if (!hasVal) {
            return true;
        }
        T v;
        if (!ArchiveValue(aR, v, aMin, aMax)) return false;
        aOpt = v;
        return true;
    }
    return false;
}

template <typename Archive, typename... Ts>
bool ArchiveVariant(Archive& aR, std::variant<Ts...>& aVar)
{
    using variant_t = std::variant<Ts...>;
    using index_t   = uint32_t;

    constexpr index_t variantSize = SafeU32(std::variant_size_v<variant_t>);
    if constexpr (IsStreamEncoder<Archive>) {
        index_t tag = static_cast<index_t>(aVar.index());
        if (!ArchiveValue(aR, tag, 0u, variantSize)) return false;
        return std::visit(
            [&](auto& aV) {
                using T = std::decay_t<decltype(aV)>;
                if constexpr (std::is_same_v<T, std::monostate>)
                    return true;
                else
                    return aV.Archive(aR);
            },
            aVar);
    } else if constexpr (IsStreamDecoder<Archive>) {
        index_t tag = 0;
        if (!ArchiveValue(aR, tag, 0u, variantSize)) return false;

        bool ok     = false;
        auto assign = [&](auto aIdx) {
            using T = std::tuple_element_t<aIdx, std::tuple<Ts...>>;
            T value;
            if constexpr (std::is_same_v<T, std::monostate>) {
                ok = true;
            } else {
                ok = value.Archive(aR);
            }
            if (ok) aVar = std::move(value);
        };
        bool found = false;
        for (index_t i = 0; i < variantSize; ++i) {
            if (tag == i) {
                ([&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    ((i == Is ? assign(std::integral_constant<std::size_t, Is>{}) : void()), ...);
                })(std::make_index_sequence<sizeof...(Ts)>{});
                found = true;
                break;
            }
        }
        return ok && found;
    }
    return false;
}

template <glm::length_t L, typename T, glm::qualifier Q, typename MinMaxT>
constexpr bool
ArchiveVector(auto& aArchive, glm::vec<L, T, Q>& aObj, const MinMaxT aMin, const MinMaxT aMax)
{
    for (std::size_t idx = 0; idx < L; ++idx) {
        if (!ArchiveValue(aArchive, aObj[idx], aMin, aMax)) return false;
    }
    return true;
}

template <typename T, glm::qualifier Q>
constexpr bool ArchiveQuaternion(auto& aArchive, glm::qua<T, Q>& aObj)
{
    for (std::size_t idx = 0; idx < 4; ++idx) {
        if (!ArchiveValue(aArchive, aObj[idx], -1.0f, 1.0f)) return false;
    }
    return true;
}
#ifndef DOCTEST_CONFIG_DISABLE
#include "test.hpp"

TEST_CASE("bitbuffer.single")
{
    BitWriter buf;
    uint32_t  value = 0b101101;

    buf.Write(value, 6);
    BitReader reader(buf.Data());

    uint32_t r = 0;

    CHECK(reader.Read(r, 6));
    CHECK_EQ(r, value);
}

TEST_CASE("bitbuffer.multiple")
{
    BitWriter             buf;
    std::vector<uint32_t> values = {1, 2, 3, 4, 5, 31};
    for (auto v : values) buf.Write(v, 5);

    BitReader reader(buf.Data());

    std::vector<uint32_t> out(values.size());
    for (auto& v : out) {
        CHECK(reader.Read(v, 5));
    }

    CHECK_EQ(out, values);
}

TEST_CASE("bitbuffer.edge_cases")
{
    BitWriter buf;
    uint32_t  max32 = 0xFFFFFFFF;

    buf.Write(max32, 32);
    buf.Write(1u, 1);

    BitReader reader(buf.Data());
    uint32_t  r = 0;

    CHECK(reader.Read(r, 32));
    CHECK_EQ(r, max32);

    CHECK(reader.Read(r, 1));
    CHECK_EQ(r, 1);
}

TEST_CASE("bitbuffer.zero")
{
    BitWriter buf;
    uint32_t  dummy = 123;

    buf.Write(dummy, 0);

    BitReader reader(buf.Data());
    uint32_t  r = 0;

    CHECK(reader.Read(r, 0));
    CHECK_EQ(r, 0);
}

TEST_CASE("bitbuffer.write_more_32")
{
    BitWriter buf;
    uint32_t  max32 = 0xFFFFFFFF;

    buf.Write(max32, 31);
    buf.Write(max32, 31);

    BitReader reader(buf.Data());

    uint32_t r = 0;

    CHECK(reader.Read(r, 31));
    CHECK_EQ(r, max32 >> 1);

    CHECK(reader.Read(r, 31));
    CHECK_EQ(r, max32 >> 1);
}

TEST_CASE("encode.int")
{
    StreamEncoder enc;

    enc.EncodeInt(42, 0, 100);
    enc.EncodeInt(-42, -100, 100);

    StreamDecoder dec(enc.Data());
    int           val = 0;

    CHECK(dec.DecodeInt(val, 0, 100));
    CHECK_EQ(val, 42);

    CHECK(dec.DecodeInt(val, -100, 100));
    CHECK_EQ(val, -42);
}

TEST_CASE("encode.unsigned")
{
    StreamEncoder enc;

    uint16_t v16 = 21;
    uint64_t v64 = 84;
    uint32_t v32 = 42;

    enc.EncodeUInt(v16, 0, 100);
    enc.EncodeUInt(v64, 0, 100);
    enc.EncodeUInt(v32, 0, 100);

    StreamDecoder dec(enc.Data());

    uint16_t r16 = 0;
    uint64_t r64 = 0;
    uint32_t r32 = 0;

    CHECK(dec.DecodeUInt(r16, 0, 100));
    CHECK_EQ(r16, v16);

    CHECK(dec.DecodeUInt(r64, 0, 100));
    CHECK_EQ(r64, v64);

    CHECK(dec.DecodeUInt(r32, 0, 100));
    CHECK_EQ(r32, v32);
}

TEST_CASE("encode.float")
{
    StreamEncoder enc;

    enc.EncodeFloat(3.14159f);
    enc.EncodeFloat(-3.141590f);

    StreamDecoder dec(enc.Data());
    float         val;

    CHECK_EQ(dec.DecodeFloat(val), true);
    CHECK_EQ(val, doctest::Approx(3.14159));

    CHECK_EQ(dec.DecodeFloat(val), true);
    CHECK_EQ(val, doctest::Approx(-3.14159));
}

TEST_CASE("encode.value")
{
    StreamEncoder enc;

    ArchiveValue(enc, 3.14159f, 0.0f, 5.0f);
    ArchiveValue(enc, -3.14159f, -5.0f, 5.0f);

    StreamDecoder dec(enc.Data());
    float         val;

    CHECK_EQ(ArchiveValue(dec, val, 0.0f, 5.0f), true);
    CHECK_EQ(val, doctest::Approx(3.14159));

    CHECK_EQ(ArchiveValue(dec, val, -5.0f, 5.0f), true);
    CHECK_EQ(val, doctest::Approx(-3.14159));
}

TEST_CASE("encode.limits")
{
    StreamEncoder enc;

    uint16_t v16 = std::numeric_limits<uint16_t>::max();
    uint64_t v64 = std::numeric_limits<uint64_t>::max();
    uint32_t v32 = std::numeric_limits<uint32_t>::max();

    enc.EncodeUInt(v16, v16 - 1, v16);
    enc.EncodeUInt(v64, v64 - 1, v64);
    enc.EncodeUInt(v32, v32 - 1, v32);

    StreamDecoder dec(enc.Data());

    uint16_t r16 = 0;
    uint64_t r64 = 0;
    uint32_t r32 = 0;

    CHECK(dec.DecodeUInt(r16, v16 - 1, v16));
    CHECK_EQ(r16, v16);

    CHECK(dec.DecodeUInt(r64, v64 - 1, v64));
    CHECK_EQ(r64, v64);

    CHECK(dec.DecodeUInt(r32, v32 - 1, v32));
    CHECK_EQ(r32, v32);
}

#endif
