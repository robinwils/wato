#pragma once

#include <renderer/primitive.hpp>
#include <vector>

class MeshPrimitive : public Primitive<PositionNormalUvVertex>
{
   public:
    using indice_type = Primitive<PositionNormalUvVertex>::indice_type;

    MeshPrimitive(
        std::vector<PositionNormalUvVertex> aVertices,
        std::vector<indice_type>            aIndices,
        Material*                           aMat)
        : Primitive(aMat, std::move(aVertices), std::move(aIndices))
    {
        InitializePrimitive();
    }
};
