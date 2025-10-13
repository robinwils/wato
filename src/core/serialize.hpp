#pragma once

#include <concepts>
#include <vector>

template <typename T, typename Out>
concept Serializable = requires(T aObj, Out& aOut) {
    { T::Serialize(aOut, aObj) } -> std::same_as<void>;
};

template <typename T, typename In>
concept Deserializable = requires(T aObj, In& aIn) {
    { T::Deserialize(aIn, aObj) } -> std::same_as<bool>;
};

template <typename T>
concept is_trivially_serializable =
    std::integral<T> || std::floating_point<T> || std::same_as<T, bool>;

template <is_trivially_serializable T>
constexpr auto Serialize(auto& aArchive, const T& aObj, std::size_t aN = 1)
{
    aArchive.template Write<T>(&aObj, aN);
}

template <is_trivially_serializable T>
constexpr auto Deserialize(auto& aArchive, T& aObj, std::size_t aN = 1)
{
    aArchive.template Read<T>(&aObj, aN);
}

template <is_trivially_serializable T>
constexpr auto Serialize(auto& aArchive, const std::vector<T>& aObj)
{
    Serialize(aArchive, aObj.size());
    for (const T& elt : aObj) {
        Serialize(aArchive, elt);
    }
}

template <is_trivially_serializable T>
constexpr auto Deserialize(auto& aArchive, std::vector<T>& aObj)
{
    using size_type = typename std::vector<T>::size_type;

    size_type n = 0;

    Deserialize(aArchive, n);
    for (size_type i = 0; i < n; ++i) {
        T elt;
        Deserialize(aArchive, elt);
        aObj.emplace_back(elt);
    }
}
