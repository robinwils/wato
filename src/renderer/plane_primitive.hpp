#pragma once

#include <renderer/primitive.hpp>

class PlanePrimitive : public Primitive
{
   public:
    PlanePrimitive()
    {
        m_vertices = {
            {{+0.5f, +0.0f, -0.5f}, {0.0f, +1.0f, 0.0f}, {0.0f, 1.0f}},
            {{-0.5f, +0.0f, -0.5f}, {0.0f, +1.0f, 0.0f}, {0.0f, 0.0f}},
            {{-0.5f, +0.0f, +0.5f}, {0.0f, +1.0f, 0.0f}, {1.0f, 0.0f}},
            {{+0.5f, +0.0f, +0.5f}, {0.0f, +1.0f, 0.0f}, {1.0f, 1.0f}},
        };

        m_indices = {
            0,
            1,
            2,  //

            0,
            2,
            3,  //
        };

        Primitive::initializePrimitive();
    }
};
