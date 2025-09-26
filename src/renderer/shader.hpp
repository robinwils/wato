#pragma once

#include <bx/bx.h>

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

    Shader(bgfx::ProgramHandle&& aHandle, uniform_map&& aUniforms)
        : mHandle(std::move(aHandle)), mUniforms(std::move(aUniforms))
    {
    }

    bgfx::UniformHandle Uniform(const char* const aName) const
    {
        BX_ASSERT(mUniforms.contains(aName), "uniform %s does not exist", aName);
        return mUniforms.at(aName);
    }

    bgfx::ProgramHandle Program() const { return mHandle; }

   private:
    bgfx::ProgramHandle mHandle;
    uniform_map         mUniforms;
};
