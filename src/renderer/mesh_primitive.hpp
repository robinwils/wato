#pragma once

#include <renderer/primitive.hpp>
#include <vector>

class MeshPrimitive : public Primitive<PositionNormalUvVertex>
{
   public:
    MeshPrimitive(std::vector<PositionNormalUvVertex> aVertices,
        std::vector<uint16_t>                         aIndices,
        Material*                                     aMat)
        : Primitive(aMat, aVertices, aIndices)
    {
        InitializePrimitive();
    }
};
