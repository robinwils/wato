#pragma once

#include <bgfx/bgfx.h>
#include <fmt/base.h>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int4.hpp>
#include <glm/gtx/string_cast.hpp>
#include <string>

struct PositionVertex {
    glm::vec3 Position;

    static bgfx::VertexLayout GetVertexLayout()
    {
        bgfx::VertexLayout vertexLayout;
        vertexLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 3, bgfx::AttribType::Float)
            .end();
        return vertexLayout;
    }
};

struct PositionNormalUvVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 Uv;

    static bgfx::VertexLayout GetVertexLayout()
    {
        bgfx::VertexLayout vertexLayout;
        vertexLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();
        return vertexLayout;
    }
};

struct PositionNormalUvBoneVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 Uv;
    glm::vec4 BoneWeights{0.0f};
    glm::vec4 BoneIndices{0.0f};

    static bgfx::VertexLayout GetVertexLayout()
    {
        bgfx::VertexLayout vertexLayout;
        vertexLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Float)
            .end();
        return vertexLayout;
    }
};

template <>
struct fmt::formatter<PositionVertex> : fmt::formatter<std::string> {
    auto format(PositionVertex aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{{{}}}", glm::to_string(aObj.Position));
    }
};

template <>
struct fmt::formatter<PositionNormalUvVertex> : fmt::formatter<std::string> {
    auto format(PositionNormalUvVertex aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "{{{} {} {}}}",
            glm::to_string(aObj.Position),
            glm::to_string(aObj.Normal),
            glm::to_string(aObj.Uv));
    }
};

template <>
struct fmt::formatter<PositionNormalUvBoneVertex> : fmt::formatter<std::string> {
    auto format(PositionNormalUvBoneVertex aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "{{{} {} {}}}",
            glm::to_string(aObj.Position),
            glm::to_string(aObj.Normal),
            glm::to_string(aObj.Uv),
            glm::to_string(aObj.BoneWeights),
            glm::to_string(aObj.BoneIndices));
    }
};
