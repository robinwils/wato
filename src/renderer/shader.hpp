#pragma once

#include <string>
#include <unordered_map>

#include "bgfx/bgfx.h"

class Shader
{
   public:
    Shader(bgfx::ProgramHandle                               &handle,
        std::unordered_map<std::string, bgfx::UniformHandle> &uniforms)
        : m_handle(handle), m_uniforms(uniforms)
    {
    }

    bgfx::UniformHandle uniform(const char *const name) const { return m_uniforms.at(name); }

    bgfx::ProgramHandle program() const { return m_handle; }

   private:
    std::unordered_map<std::string, bgfx::UniformHandle> m_uniforms;
    bgfx::ProgramHandle                                  m_handle;
};
