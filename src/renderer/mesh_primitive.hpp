#pragma once

#include <renderer/primitive.hpp>
#include <vector>

class MeshPrimitive : public Primitive
{
   public:
    MeshPrimitive(std::vector<PositionNormalUvVertex> _vertices,
        std::vector<uint16_t>                         _indices,
        const Material&                               _mat)
        : Primitive(_mat, _vertices, _indices)
    {
        initializePrimitive();
    }
};
