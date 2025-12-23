#include "renderer/model.hpp"

#include "core/types.hpp"
#include "renderer/instance_buffer.hpp"

void Model::Submit(glm::mat4 aModelMatrix, uint64_t aState)
{
    for (const auto& mesh : mMeshes) {
        bgfx::setTransform(glm::value_ptr(aModelMatrix));

        bgfx::setState(aState);
        std::visit(
            VariantVisitor{
                [&](const auto& aPrimitive) { aPrimitive->Submit(); },
            },
            mesh);
    }
}

void Model::Submit(const InstanceBuffer& aBuffer, uint64_t aState)
{
    for (const auto& mesh : mMeshes) {
        aBuffer.SetBuffer();
        bgfx::setState(aState);
        std::visit(
            VariantVisitor{
                [&](const auto& aPrimitive) { aPrimitive->Submit(); },
            },
            mesh);
    }
}
