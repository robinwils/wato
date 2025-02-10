#pragma once

#include <renderer/primitive.hpp>
#include <vector>

class MeshPrimitive : public Primitive
{
   public:
    std::vector<Material> materials;

    MeshPrimitive() : Primitive() {}
    MeshPrimitive(std::vector<PositionNormalUvVertex> vertices, std::vector<uint16_t> indices)
        : Primitive(vertices, indices)
    {
        initializePrimitive();
    }
};
