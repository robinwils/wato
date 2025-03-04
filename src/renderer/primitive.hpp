#pragma once

#include <bgfx/bgfx.h>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <renderer/material.hpp>
#include <utility>
#include <vector>

#include "core/sys.hpp"
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
    Primitive(const Material& _material) : m_material(_material), m_is_initialized(false) {}
    Primitive(const Material&               _material,
        std::vector<PositionNormalUvVertex> vertices,
        std::vector<uint16_t>               indices)
        : m_vertices(vertices), m_indices(indices), m_material(_material)
    {
    }
    Primitive(const Primitive& other) noexcept
        : m_vertices(other.m_vertices),
          m_indices(other.m_indices),
          m_material(other.m_material),
          m_is_initialized(other.m_is_initialized),
          m_vertex_buffer_handle(other.m_vertex_buffer_handle),
          m_index_buffer_handle(other.m_index_buffer_handle)
    {
    }
    Primitive(Primitive&& other) noexcept
        : m_vertices(std::move(other.m_vertices)),
          m_indices(std::move(other.m_indices)),
          m_material(std::move(other.m_material)),
          m_vertex_buffer_handle(other.m_vertex_buffer_handle),
          m_index_buffer_handle(other.m_index_buffer_handle),

          m_is_initialized(other.m_is_initialized)
    {
        other.m_vertex_buffer_handle.idx = bgfx::kInvalidHandle;
        other.m_index_buffer_handle.idx  = bgfx::kInvalidHandle;
        other.m_is_initialized           = false;
    }

    Primitive& operator=(Primitive& other)
    {
        m_vertex_buffer_handle = other.m_vertex_buffer_handle;
        m_index_buffer_handle  = other.m_index_buffer_handle;
        m_vertices             = std::move(other.m_vertices);
        m_indices              = std::move(other.m_indices);
        return *this;
    }
    Primitive& operator=(Primitive&& other)
    {
        m_vertex_buffer_handle = other.m_vertex_buffer_handle;
        m_index_buffer_handle  = other.m_index_buffer_handle;
        m_vertices             = std::move(other.m_vertices);
        m_indices              = std::move(other.m_indices);
        return *this;
    }
    virtual ~Primitive() { destroyPrimitive(); }

    virtual void submit(uint8_t discard_states = BGFX_DISCARD_ALL) const
    {
        m_material.submit();
        assert(m_is_initialized);

        bgfx::setVertexBuffer(0, m_vertex_buffer_handle);
        bgfx::setIndexBuffer(m_index_buffer_handle);

        bgfx::submit(0, m_material.program, bgfx::ViewMode::Default, discard_states);
    }

   protected:
    std::vector<PositionNormalUvVertex> m_vertices;
    std::vector<uint16_t>               m_indices;
    Material                            m_material;

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
        m_index_buffer_handle = bgfx::createIndexBuffer(
            bgfx::makeRef(m_indices.data(), sizeof(uint16_t) * m_indices.size()));

        m_is_initialized = true;
    }

   private:
    virtual void destroyPrimitive()
    {
        if (m_is_initialized) {
            DBG("destroying vertex buffer %d and index buffer %d",
                m_vertex_buffer_handle.idx,
                m_index_buffer_handle.idx);
            bgfx::destroy(m_vertex_buffer_handle);
            bgfx::destroy(m_index_buffer_handle);
        }
    }
};
