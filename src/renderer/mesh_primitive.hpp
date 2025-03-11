#pragma once

#include <renderer/primitive.hpp>
#include <vector>

class MeshPrimitive : public Primitive<PositionNormalUvVertex>
{
   public:
    MeshPrimitive(std::vector<PositionNormalUvVertex> _vertices,
        std::vector<uint16_t>                         _indices,
        Material*                                     _mat)
        : Primitive(_mat, _vertices, _indices)
    {
        initializePrimitive();
    }
};
