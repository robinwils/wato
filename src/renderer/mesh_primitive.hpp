#pragma once

#include <renderer/primitive.hpp>
#include <vector>

struct PositionNormalUvBoneVertex {
    glm::vec3  Position;
    glm::vec3  Normal;
    glm::vec2  Uv;
    glm::ivec4 BoneIndices{-1};
    glm::vec4  BoneWeights;

    static bgfx::VertexLayout GetVertexLayout()
    {
        bgfx::VertexLayout vertexLayout;
        vertexLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Int16)
            .add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float)
            .end();
        return vertexLayout;
    }
};

class MeshPrimitive : public Primitive<PositionNormalUvBoneVertex>
{
   public:
    using indice_type = Primitive<PositionNormalUvBoneVertex>::indice_type;

    MeshPrimitive(
        std::vector<PositionNormalUvBoneVertex> aVertices,
        std::vector<indice_type>                aIndices,
        Material*                               aMat)
        : Primitive(aMat, std::move(aVertices), std::move(aIndices))
    {
        InitializePrimitive();
    }
};
