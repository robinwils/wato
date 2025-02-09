#pragma once

#include <bgfx/bgfx.h>

#include <algorithm>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <renderer/material.hpp>
#include <utility>
#include <vector>

#include "glm/fwd.hpp"

struct PositionNormalUvVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    static bgfx::VertexLayout getVertexLayout()
    {
        bgfx::VertexLayout vertex_layout;
        vertex_layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();
        return vertex_layout;
    }
};

class Primitive
{
   public:
    Primitive() : m_is_initialized(false) {}
    Primitive(std::vector<PositionNormalUvVertex> vertices, std::vector<uint16_t> indices)
        : m_vertices(std::move(vertices)), m_indices(std::move(indices))
    {
    }
    virtual ~Primitive() { destroyPrimitive(); }

    virtual void submitPrimitive(const Material& material, uint8_t discard_states = BGFX_DISCARD_ALL) const
    {
        assert(m_is_initialized);

        bgfx::setVertexBuffer(0, m_vertex_buffer_handle);
        bgfx::setIndexBuffer(m_index_buffer_handle);

        bgfx::submit(0, material.program, bgfx::ViewMode::Default, discard_states);
    }

   protected:
    std::vector<PositionNormalUvVertex> m_vertices;
    std::vector<uint16_t>               m_indices;

    bool m_is_initialized;

    bgfx::VertexBufferHandle m_vertex_buffer_handle;
    bgfx::IndexBufferHandle  m_index_buffer_handle;

    virtual void initializePrimitive()
    {
        assert(!m_vertices.empty());
        assert(!m_indices.empty());

        const bgfx::VertexLayout vertex_layout = PositionNormalUvVertex::getVertexLayout();

        m_vertex_buffer_handle = bgfx::createVertexBuffer(
            bgfx::makeRef(m_vertices.data(), sizeof(PositionNormalUvVertex) * m_vertices.size()),
            vertex_layout);
        m_index_buffer_handle =
            bgfx::createIndexBuffer(bgfx::makeRef(m_indices.data(), sizeof(uint16_t) * m_indices.size()));

        m_is_initialized = true;
    }

   private:
    virtual void destroyPrimitive()
    {
        assert(m_is_initialized);

        bgfx::destroy(m_vertex_buffer_handle);
        bgfx::destroy(m_index_buffer_handle);
    }
};
