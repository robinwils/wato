#pragma once

#include <bgfx/bgfx.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "core/types.hpp"

struct InstanceData {
    glm::mat4 Transform;
};

class InstanceBuffer
{
   public:
    void Clear() { mInstances.clear(); }

    void Add(const glm::mat4& aTransform) { mInstances.push_back({aTransform}); }

    [[nodiscard]] uint32_t Count() const { return SafeU32(mInstances.size()); }

    [[nodiscard]] bool Empty() const { return mInstances.empty(); }

    [[nodiscard]] bool Allocate()
    {
        if (mInstances.empty()) {
            return false;
        }

        uint32_t numInstances = SafeU32(mInstances.size());
        uint16_t stride       = sizeof(InstanceData);

        // Check if we can allocate enough instances
        if (bgfx::getAvailInstanceDataBuffer(numInstances, stride) < numInstances) {
            return false;
        }

        bgfx::allocInstanceDataBuffer(&mIDB, numInstances, stride);

        std::memcpy(mIDB.data, mInstances.data(), numInstances * stride);

        return true;
    }

    void SetBuffer() const { bgfx::setInstanceDataBuffer(&mIDB); }

   private:
    std::vector<InstanceData> mInstances;
    bgfx::InstanceDataBuffer  mIDB;
};
