#pragma once

#include <bgfx/bgfx.h>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int4.hpp>
#include <variant>

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
    glm::vec3  Position;
    glm::vec3  Normal;
    glm::vec2  Uv;
    glm::vec4  BoneWeights{0};
    glm::ivec4 BoneIndices{-1};

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
