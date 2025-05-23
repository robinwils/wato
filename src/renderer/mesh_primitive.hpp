#pragma once

#include "renderer/primitive.hpp"

template <typename VL>
class MeshPrimitive : public Primitive<VL>
{
   public:
    using indice_type = Primitive<VL>::indice_type;

    MeshPrimitive(std::vector<VL> aVertices, std::vector<indice_type> aIndices, Material* aMat)
        : Primitive<VL>(aMat, std::move(aVertices), std::move(aIndices))
    {
        InitializePrimitive();
    }
    void InitializePrimitive() override { Primitive<VL>::InitializePrimitive(); }
};
