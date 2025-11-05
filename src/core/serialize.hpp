#pragma once

#include <bx/bx.h>

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <glm/detail/qualifier.hpp>
#include <glm/gtc/type_ptr.hpp>
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
#endif

#if defined(_MSC_VER)
constexpr inline std::uint16_t byteswap(std::uint16_t aX) { return _byteswap_ushort(aX); }
constexpr inline std::uint32_t byteswap(std::uint32_t aX) { return _byteswap_ulong(aX); }
constexpr inline std::uint64_t byteswap(std::uint64_t aX) { return _byteswap_uint64(aX); }
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

class BitWriter
{
   public:
    using word       = uint32_t;
    using bit_buffer = std::vector<word>;
    using bit_stream = std::span<word>;

    BitWriter() : mScratch(0), mCurBit(0) {}
    BitWriter(const BitWriter& aBuf) : mBuf(aBuf.mBuf), mScratch(0), mCurBit(0) {}

    void Write(uint32_t aData, uint32_t aN)
    {
        BX_ASSERT(aN <= 32, "cannot write more than 32 bits");

        // mask input data to be exactly aN bits
        aData    &= (1lu << aN) - 1lu;
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
    using word       = uint32_t;
    using bit_stream = std::span<word>;
    using bit_buffer = std::vector<word>;

    BitReader() : mScratch(0), mCurBit(0) {}
    BitReader(bit_stream&& aBits)
        : mBuf(std::move(aBits)), mScratch(0), mCurBit(0), mNext(mBuf.data())
    {
    }
    BitReader(bit_buffer& aBits)
        : mBuf(std::span(aBits)), mScratch(0), mCurBit(0), mNext(mBuf.data())
    {
    }

    std::optional<uint32_t> Read(uint32_t aN)
    {
        BX_ASSERT(aN <= 32, "cannot write more than 32 bits");

        // not enough bits in scratch, read next word
        if (aN > mCurBit) {
            if (!mNext || mNext >= mBuf.data() + mBuf.size()) {
                return std::nullopt;
            }
            uint32_t next = *mNext++;

            if constexpr (std::endian::native == std::endian::little) {
                mScratch |= SafeU64(next) << mCurBit;
            } else {
                mScratch |= SafeU64(bswap_any(next)) << mCurBit;
            }
            mCurBit += 32;
        }

        uint32_t data   = SafeU32(mScratch & (1lu << aN) - 1);
        mScratch      >>= aN;
        mCurBit        -= aN;

        return data;
    }

   private:
    bit_stream mBuf;
    uint64_t   mScratch;
    uint32_t   mCurBit;
    word*      mNext;
};

class StreamEncoder
{
   public:
    StreamEncoder() = default;

    void EncodeBool(bool aVal) { mBits.Write(aVal ? 1 : 0, 1); }

    void EncodeInt(int32_t aVal, int32_t aMin, int32_t aMax)
    {
        BX_ASSERT(aMin < aMax, "min is higher or equal than max");
        BX_ASSERT(aVal >= aMin, "value is less than min");
        BX_ASSERT(aVal <= aMax, "value is more than max");
        uint32_t value = uint32_t(aVal - aMin);
        auto     diff  = int64_t(aMax) - int64_t(aMin);
        mBits.Write(value, uint32_t(std::bit_width(uint32_t(diff))));
    }

    void EncodeUInt(uint32_t aVal, uint32_t aMin, uint32_t aMax)
    {
        BX_ASSERT(aMin < aMax, "min is higher or equal than max");
        BX_ASSERT(aVal >= aMin, "value is less than min");
        BX_ASSERT(aVal <= aMax, "value is more than max");

        mBits.Write(aVal - aMin, uint32_t(std::bit_width(aMax - aMin)));
    }

    void EncodeFloat16(float aVal, float aMin, float aMax)
    {
        auto bits = PackFloat<uint16_t>(aVal, aMin, aMax);
        mBits.Write(bits, 16u);
    }

    BitWriter::bit_buffer& Data() { return mBits.Data(); }

   protected:
    template <typename IntT>
    uint32_t PackFloat(float aVal, float aMin, float aMax)
    {
        aVal       = std::clamp(aVal, aMin, aMax);
        float norm = (aVal - aMin) / (aMax - aMin);
        return uint32_t(norm * std::numeric_limits<IntT>::max() + 0.5f);
    }
    BitWriter mBits;
};

class StreamDecoder
{
   public:
    StreamDecoder() = default;
    StreamDecoder(BitReader::bit_stream&& aBits) : mBits(std::move(aBits)) {}
    StreamDecoder(BitReader::bit_buffer& aBits) : mBits(aBits) {}

    bool DecodeBool(bool& aVal)
    {
        if (auto r = mBits.Read(1); r) {
            aVal = r == 1;
            return true;
        } else {
            return false;
        }
    }
    bool DecodeInt(int32_t& aVal, int32_t aMin, int32_t aMax)
    {
        BX_ASSERT(aMin < aMax, "min is higher or equal than max");
        BX_ASSERT(aVal >= aMin, "value is less than min");
        BX_ASSERT(aVal <= aMax, "value is more than max");

        uint32_t bits = uint32_t(std::bit_width(uint32_t(aMax - aMin)));
        if (auto r = mBits.Read(bits); r) {
            aVal = int32_t(*r) + aMin;
            return true;
        } else {
            return false;
        }
    }

    bool DecodeUInt(uint32_t& aVal, uint32_t aMin, uint32_t aMax)
    {
        BX_ASSERT(aMin < aMax, "min is higher or equal than max");
        BX_ASSERT(aVal >= aMin, "value is less than min");
        BX_ASSERT(aVal <= aMax, "value is more than max");

        uint32_t bits = uint32_t(std::bit_width(aMax - aMin));
        if (auto r = mBits.Read(bits); r) {
            aVal = *r + aMin;
            return true;
        } else {
            return false;
        }
    }

    bool DecodeFloat16(float& aVal, float aMin, float aMax)
    {
        BX_ASSERT(aMin < aMax, "min is higher or equal than max");
        BX_ASSERT(aVal >= aMin, "value is less than min");
        BX_ASSERT(aVal <= aMax, "value is more than max");

        if (auto r = mBits.Read(16u); r) {
            aVal = UnpackFloat<uint16_t>(*r, aMin, aMax);
            return true;
        } else {
            return false;
        }
    }

   protected:
    template <typename IntT>
    float UnpackFloat(uint32_t aPacked, float aMin, float aMax)
    {
        float value = float(aPacked) / float(std::numeric_limits<IntT>::max());
        return value * (aMax - aMin) + aMin;
    }

    BitReader mBits;
};

template <typename T>
concept IsStreamEncoder = requires(T t) {
    { t.EncodeInt(std::declval<int>(), std::declval<int>(), std::declval<int>()) };
    {
        t.EncodeUInt16(std::declval<uint16_t>(), std::declval<uint32_t>(), std::declval<uint32_t>())
    };
    { t.EncodeUInt(std::declval<uint32_t>(), std::declval<uint32_t>(), std::declval<uint32_t>()) };
    { t.EncodeFloat16(std::declval<float>(), std::declval<float>(), std::declval<float>()) };
};

template <typename T>
concept IsStreamDecoder = requires(T t, int32_t& vi, uint16_t& vu16, uint32_t& vu, float& vf) {
    { t.DecodeInt(vi, std::declval<int>(), std::declval<int>()) } -> std::same_as<bool>;
    {
        t.DecodeUInt16(vu16, std::declval<uint16_t>(), std::declval<uint32_t>())
    } -> std::same_as<bool>;
    { t.DecodeUInt(vu, std::declval<uint32_t>(), std::declval<uint32_t>()) } -> std::same_as<bool>;
    { t.DecodeFloat16(vf, std::declval<float>(), std::declval<float>()) } -> std::same_as<bool>;
};

template <typename Archive>
    requires IsStreamEncoder<Archive>
bool ArchiveBool(Archive& aR, const bool& aValue)
{
    aR.EncodeBool(aValue);
    return true;
}

template <typename Archive>
    requires IsStreamDecoder<Archive>
bool ArchiveBool(Archive& aR, bool& aValue)
{
    if (!aR.DecodeBool(aValue)) return false;
    return true;
}

template <typename Archive, typename T, typename MinMaxT>
    requires IsStreamEncoder<Archive>
bool ArchiveValue(Archive& aR, const T& aValue, MinMaxT aMin, MinMaxT aMax)
{
    using value_t = std::remove_cv_t<T>;
    if constexpr (std::is_enum_v<value_t>) {
        using U = std::underlying_type_t<value_t>;
        spdlog::info("encoding enum {}", static_cast<U>(aValue));
        return ArchiveValue(aR, static_cast<U>(aValue), static_cast<U>(aMin), static_cast<U>(aMax));
    } else if constexpr (std::is_same_v<value_t, float>) {
        spdlog::info("encoding float {}", aValue);
        aR.EncodeFloat16(aValue, aMin, aMax);
        return true;
    } else if constexpr (std::is_integral_v<value_t> && std::is_signed_v<value_t>) {
        spdlog::info("encoding int {}", aValue);
        aR.EncodeInt(aValue, aMin, aMax);
        return true;
    } else if constexpr (
        std::is_integral_v<value_t> && std::is_unsigned_v<value_t> && sizeof(T) == 2) {
        spdlog::info("encoding uint16 {}", aValue);
        aR.EncodeUInt16(aValue, aMin, aMax);
        return true;
    } else if constexpr (std::is_integral_v<value_t> && std::is_unsigned_v<value_t>) {
        spdlog::info("encoding uint32 {}", aValue);
        aR.EncodeUInt(aValue, aMin, aMax);
        return true;
    }
    return false;
}

template <typename Archive, typename T, typename MinMaxT>
    requires IsStreamDecoder<Archive>
bool ArchiveValue(Archive& aR, T& aValue, MinMaxT aMin, MinMaxT aMax)
{
    using value_t = std::remove_cv_t<T>;

    if constexpr (std::is_enum_v<value_t>) {
        using U = std::underlying_type_t<value_t>;
        U tmp;
        if (!ArchiveValue(aR, tmp, static_cast<U>(aMin), static_cast<U>(aMax))) return false;
        aValue = static_cast<value_t>(tmp);
        spdlog::info("decoded enum {}", tmp);
        return true;
    } else if constexpr (std::is_same_v<value_t, float>) {
        if (!aR.DecodeFloat16(aValue, aMin, aMax)) return false;
        if (aValue > aMax || aValue < aMin) return false;
        spdlog::info("decoded float {}", aValue);
        return true;
    } else if constexpr (std::is_integral_v<value_t> && std::is_signed_v<value_t>) {
        if (!aR.DecodeInt(aValue, aMin, aMax)) return false;
        if (aValue > aMax || aValue < aMin) return false;
        spdlog::info("decoded int {}", aValue);
        return true;
    } else if constexpr (
        std::is_integral_v<value_t> && std::is_unsigned_v<value_t> && sizeof(T) == 2) {
        aR.DecodeUInt16(aValue, aMin, aMax);
        spdlog::info("decoded uint16 {}", aValue);
        return true;
    } else if constexpr (std::is_integral_v<value_t> && std::is_unsigned_v<value_t>) {
        if (!aR.DecodeUInt(aValue, aMin, aMax)) return false;
        if (aValue > aMax || aValue < aMin) return false;
        spdlog::info("decoded uint32 {}", aValue);
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
        return std::visit([&](auto& aV) { return aV.Archive(aR); }, aVar);
    } else if constexpr (IsStreamDecoder<Archive>) {
        index_t tag = 0;
        if (!ArchiveValue(aR, tag, 0u, variantSize)) return false;

        bool ok     = false;
        auto assign = [&](auto aIdx) {
            using T = std::tuple_element_t<aIdx, std::tuple<Ts...>>;
            T value;
            ok = value.Archive(aR);
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

template <class T>
concept not_bool = !std::same_as<std::remove_cv_t<T>, bool>;

template <class T>
concept fixed_size_scalar =
    (std::is_integral_v<T> || std::is_floating_point_v<T>)
    && (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

template <class T>
concept wire_scalar = fixed_size_scalar<T> && not_bool<T>;

template <class E>
concept fixed_enum =
    std::is_enum_v<E>
    && (sizeof(std::underlying_type_t<E>) == 1 || sizeof(std::underlying_type_t<E>) == 2
        || sizeof(std::underlying_type_t<E>) == 4 || sizeof(std::underlying_type_t<E>) == 8);

template <class T>
concept wire_native = wire_scalar<T> || fixed_enum<T>;

template <wire_native T>
constexpr auto Serialize(auto& aArchive, const T& aObj, std::size_t aN = 1)
{
    aArchive.Write(&aObj, aN);
}

template <wire_native T>
constexpr auto Deserialize(auto& aArchive, T& aObj, std::size_t aN = 1)
{
    aArchive.template Read<T>(&aObj, aN);
}

template <wire_native T>
constexpr auto Serialize(auto& aArchive, const std::vector<T>& aObj)
{
    Serialize(aArchive, aObj.size());
    if (!aObj.empty()) {
        aArchive.Write(aObj.begin(), aObj.size());
    }
}

template <typename T>
    requires(!wire_native<T>)
constexpr auto Serialize(auto& aArchive, const std::vector<T>& aObj)
{
    Serialize(aArchive, aObj.size());
    for (const T& action : aObj) {
        T::Serialize(aArchive, action);
    }
}

template <wire_native T>
constexpr auto Deserialize(auto& aArchive, std::vector<T>& aObj)
{
    using size_type = typename std::vector<T>::size_type;

    size_type n = 0;

    Deserialize(aArchive, n);
    if (n > 0) {
        aObj.resize(n);
        aArchive.template Read<T>(aObj.begin(), n);
    }
}

template <typename T>
    requires(!wire_native<T>)
constexpr auto Deserialize(auto& aArchive, std::vector<T>& aObj)
{
    using ST = typename std::vector<T>::size_type;

    ST n = 0;

    Deserialize(aArchive, n);
    for (ST idx = 0; idx < n; idx++) {
        T v;
        if (!T::Deserialize(aArchive, v)) {
            throw std::runtime_error("cannot deserialize action");
        }
        aObj.push_back(v);
    }
}

template <glm::length_t L, typename T, glm::qualifier Q>
constexpr auto Serialize(auto& aArchive, const glm::vec<L, T, Q>& aObj)
{
    aArchive.Write(glm::value_ptr(aObj), L);
}

template <glm::length_t L, typename T, glm::qualifier Q>
constexpr auto Deserialize(auto& aArchive, glm::vec<L, T, Q>& aObj)
{
    aArchive.template Read<T>(glm::value_ptr(aObj), L);
}

template <glm::length_t L, typename T, glm::qualifier Q, typename MinMaxT>
constexpr bool
ArchiveVector(auto& aArchive, glm::vec<L, T, Q>& aObj, const MinMaxT aMin, const MinMaxT aMax)
{
    for (std::size_t idx = 0; idx < L; ++idx) {
        if (!ArchiveValue(aArchive, aObj[idx], aMin, aMax)) {
            return false;
        }
    }
    return true;
}

template <glm::length_t L, typename T, glm::qualifier Q, typename MinMaxT>
constexpr bool
ArchiveVector(auto& aArchive, const glm::vec<L, T, Q>& aObj, const MinMaxT aMin, const MinMaxT aMax)
{
    for (std::size_t idx = 0; idx < L; ++idx) {
        if (!ArchiveValue(aArchive, aObj[idx], aMin, aMax)) {
            return false;
        }
    }
    return true;
}

template <typename T, glm::qualifier Q>
constexpr bool ArchiveQuaternion(auto& aArchive, glm::qua<T, Q>& aObj)
{
    for (std::size_t idx = 0; idx < 4; ++idx) {
        if (!ArchiveValue(aArchive, aObj[idx], -1.0f, 1.0f)) {
            return false;
        }
    }
    return true;
}

template <typename T, glm::qualifier Q>
constexpr auto Serialize(auto& aArchive, const glm::qua<T, Q>& aObj)
{
    aArchive.Write(glm::value_ptr(aObj), 4);
}

template <typename T, glm::qualifier Q>
constexpr auto Deserialize(auto& aArchive, glm::qua<T, Q>& aObj)
{
    aArchive.template Read<T>(glm::value_ptr(aObj), 4);
}

#ifndef DOCTEST_CONFIG_DISABLE
#include "test.hpp"

TEST_CASE("bitbuffer.single")
{
    BitWriter buf;
    uint32_t  value = 0b101101;

    buf.Write(value, 6);
    BitReader reader(buf.Data());

    CHECK_EQ(reader.Read(6), value);
}

TEST_CASE("bitbuffer.multiple")
{
    BitWriter             buf;
    std::vector<uint32_t> values = {1, 2, 3, 4, 5, 31};
    for (auto v : values) buf.Write(v, 5);

    BitReader reader(buf.Data());

    std::vector<uint32_t> out(values.size());
    for (auto& v : out) {
        if (auto r = reader.Read(5); r) {
            v = *r;
        } else {
            break;
        }
    }
    CHECK(out == values);
}

TEST_CASE("bitbuffer.edge_cases")
{
    BitWriter buf;
    uint32_t  max32 = 0xFFFFFFFF;

    buf.Write(max32, 32);
    buf.Write(1, 1);

    BitReader reader(buf.Data());

    CHECK_EQ(reader.Read(32), max32);
    CHECK_EQ(reader.Read(1), 1);
}

TEST_CASE("bitbuffer.zero")
{
    BitWriter buf;
    uint32_t  dummy = 123;

    buf.Write(dummy, 0);

    BitReader reader(buf.Data());
    CHECK_EQ(reader.Read(0), 0);
}

TEST_CASE("bitbuffer.write_more_32")
{
    BitWriter buf;
    uint32_t  max32 = 0xFFFFFFFF;

    buf.Write(max32, 31);
    buf.Write(max32, 31);

    BitReader reader(buf.Data());
    CHECK_EQ(max32 >> 1, reader.Read(31));
    CHECK_EQ(max32 >> 1, reader.Read(31));
}

TEST_CASE("encode.int")
{
    StreamEncoder enc;

    enc.EncodeInt(42, 0, 100);
    enc.EncodeInt(-42, -100, 100);

    StreamDecoder dec(enc.Data());
    int           val = 0;

    CHECK_EQ(dec.DecodeInt(val, 0, 100), true);
    CHECK_EQ(val, 42);

    CHECK_EQ(dec.DecodeInt(val, -100, 100), true);
    CHECK_EQ(val, -42);
}

TEST_CASE("encode.float")
{
    StreamEncoder enc;

    enc.EncodeFloat16(3.14159f, 0.0f, 5.0f);
    enc.EncodeFloat16(-3.14159f, -5.0f, 5.0f);

    StreamDecoder dec(enc.Data());
    float         val;

    CHECK_EQ(dec.DecodeFloat16(val, 0.0f, 5.0f), true);
    CHECK_EQ(val, doctest::Approx(3.14159));

    CHECK_EQ(dec.DecodeFloat16(val, -5.0f, 5.0f), true);
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

#endif
