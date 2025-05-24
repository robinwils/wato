#include "renderer/model.hpp"

#include "core/types.hpp"

void Model::Submit(glm::mat4 aModelMatrix, uint64_t aState)
{
    for (const auto& mesh : mMeshes) {
        // Set model matrix for rendering.
        bgfx::setTransform(glm::value_ptr(aModelMatrix));

        // Set render states.
        bgfx::setState(aState);
        std::visit(
            VariantVisitor{
                [&](const auto& aPrimitive) { aPrimitive->Submit(); },
            },
            mesh);
    }
}
