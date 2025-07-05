#pragma once

#include <string>
#include <unordered_map>

#include "bgfx/bgfx.h"

struct UniformDesc {
    bgfx::UniformType::Enum Type;
    uint16_t                Number{1};
};

class Shader
{
   public:
    using uniform_map = std::unordered_map<std::string, bgfx::UniformHandle>;

    Shader(const bgfx::ProgramHandle& aHandle, const uniform_map& aUniforms)
        : mUniforms(aUniforms), mHandle(aHandle)
    {
    }

    bgfx::UniformHandle Uniform(const char* const aName) const { return mUniforms.at(aName); }

    bgfx::ProgramHandle Program() const { return mHandle; }

   private:
    uniform_map         mUniforms;
    bgfx::ProgramHandle mHandle;
};
