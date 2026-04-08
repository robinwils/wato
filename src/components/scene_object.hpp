#pragma once

#include <entt/core/hashed_string.hpp>
#include <glaze/core/common.hpp>
#include <glaze/core/wrappers.hpp>

struct SceneObject {
    entt::hashed_string ModelHash;
    std::string         Name;

    void SetName(const std::string& aName)
    {
        Name      = aName;
        ModelHash = entt::hashed_string{Name.data()};
    }

    [[nodiscard]] const char* WriteName() const { return ModelHash.data(); }
};

template <>
struct glz::meta<SceneObject> {
    using T = SceneObject;

    // NOLINTBEGIN(readability-identifier-naming)
    static constexpr auto value = glz::object("Name", glz::custom<&T::SetName, &T::WriteName>);
    // NOLINTEND(readability-identifier-naming)
};
