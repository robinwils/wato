#pragma once

#include <concepts>
#include <cstdint>
#include <glm/detail/qualifier.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>

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

template <typename T, typename Out>
concept Serializable = requires(T aObj, Out& aOut) {
    { T::Serialize(aOut, aObj) } -> std::same_as<void>;
};

template <typename T, typename In>
concept Deserializable = requires(T aObj, In& aIn) {
    { T::Deserialize(aIn, aObj) } -> std::same_as<bool>;
};

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
