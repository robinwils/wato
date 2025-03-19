#pragma once

#include <string>
#include <unordered_map>

#include "bgfx/bgfx.h"

class Shader
{
   public:
    Shader(bgfx::ProgramHandle                               &aHandle,
        std::unordered_map<std::string, bgfx::UniformHandle> &aUniforms)
        : mUniforms(aUniforms), mHandle(aHandle)
    {
    }

    bgfx::UniformHandle Uniform(const char *const aName) const { return mUniforms.at(aName); }

    bgfx::ProgramHandle Program() const { return mHandle; }

   private:
    std::unordered_map<std::string, bgfx::UniformHandle> mUniforms;
    bgfx::ProgramHandle                                  mHandle;
};
