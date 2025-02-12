#pragma once

#include <renderer/primitive.hpp>
#include <vector>

class MeshPrimitive : public Primitive
{
   public:
    MeshPrimitive(std::vector<PositionNormalUvVertex> vertices, std::vector<uint16_t> indices, const Material& _mat)
        : Primitive(vertices, indices), material(_mat)
    {
        initializePrimitive();
    }

   protected:
    Material material;
};
