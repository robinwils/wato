#pragma once

#include <renderer/primitive.hpp>

/*
struct PosColorVertex
{
    float m_x;
    float m_y;
    float m_z;
    uint32_t m_abgr;

    static void init()
    {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
    };

    static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorVertex::ms_layout;


static PosColorVertex s_cubeVertices[] =
{
    {-1.0f,  1.0f,  1.0f, 0xff000000 },
    { 1.0f,  1.0f,  1.0f, 0xff0000ff },
    {-1.0f, -1.0f,  1.0f, 0xff00ff00 },
    { 1.0f, -1.0f,  1.0f, 0xff00ffff },
    {-1.0f,  1.0f, -1.0f, 0xffff0000 },
    { 1.0f,  1.0f, -1.0f, 0xffff00ff },
    {-1.0f, -1.0f, -1.0f, 0xffffff00 },
    { 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t s_cubeTriList[] =
{
    0, 1, 2, // 0
    1, 3, 2,
    4, 6, 5, // 2
    5, 6, 7,
    0, 2, 4, // 4
    4, 2, 6,
    1, 5, 3, // 6
    5, 7, 3,
    0, 4, 1, // 8
    4, 5, 1,
    2, 3, 6, // 10
    6, 3, 7,
};
*/

class CubePrimitive : public Primitive
{
public:
    CubePrimitive()
    {
		m_vertices = {
			{{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}},
	        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}},
	        {{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}},
	        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}},
	        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
	        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
	        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
	        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
		};

        m_triangle_list = {
            0, 1, 2, // 0
            1, 3, 2,
            4, 6, 5, // 2
            5, 6, 7,
            0, 2, 4, // 4
            4, 2, 6,
            1, 5, 3, // 6
            5, 7, 3,
            0, 4, 1, // 8
            4, 5, 1,
            2, 3, 6, // 10
            6, 3, 7,
        };

        Primitive::initializePrimitive();
    }
};
