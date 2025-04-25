#pragma once

#include "core/snapshot.hpp"

template <typename T>
concept Serializable = requires(T t, ByteOutputArchive& outArchive) {
    { T::Serialize(outArchive, t) } -> std::same_as<void>;
};

template <typename T>
concept Deserializable = requires(T t, ByteInputArchive& inArchive) {
    { T::Deserialize(inArchive, t) } -> std::same_as<bool>;
};
