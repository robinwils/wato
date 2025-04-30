#pragma once

#include <concepts>

template <typename T, typename Out>
concept Serializable = requires(T aObj, Out& aOut) {
    { T::Serialize(aOut, aObj) } -> std::same_as<void>;
};

template <typename T, typename In>
concept Deserializable = requires(T aObj, In& aIn) {
    { T::Deserialize(aIn, aObj) } -> std::same_as<bool>;
};
