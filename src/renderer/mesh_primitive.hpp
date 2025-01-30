#pragma once

#include<vector>
#include <renderer/primitive.hpp>


class MeshPrimitive : public Primitive
{
public:
	std::vector<Material> materials;

	MeshPrimitive(std::vector<Material> materials)
	: materials(materials)
	{
		initializePrimitive();
	};
};

