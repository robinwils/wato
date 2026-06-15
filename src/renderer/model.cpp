#include "renderer/model.hpp"

#include "core/types.hpp"
#include "renderer/instance_buffer.hpp"

void Model::Submit(wato::ViewId aView, glm::mat4 aModelMatrix, uint64_t aState)
{
    for (const auto& mesh : mMeshes) {
        bgfx::setTransform(glm::value_ptr(aModelMatrix));

        bgfx::setState(aState);
        std::visit(
            VariantVisitor{
                [&](const auto& aPrimitive) { aPrimitive->Submit(aView); },
            },
            mesh);
    }
}

void Model::Submit(wato::ViewId aView, const InstanceBuffer& aBuffer, uint64_t aState)
{
    for (const auto& mesh : mMeshes) {
        aBuffer.SetBuffer();
        bgfx::setState(aState);
        std::visit(
            VariantVisitor{
                [&](const auto& aPrimitive) { aPrimitive->Submit(aView); },
            },
            mesh);
    }
}

void Model::Submit(
    wato::ViewId        aView,
    bgfx::ProgramHandle aProgram,
    glm::mat4           aModelMatrix,
    uint64_t            aState)
{
    for (const auto& mesh : mMeshes) {
        bgfx::setTransform(glm::value_ptr(aModelMatrix));

        bgfx::setState(aState);
        std::visit(
            VariantVisitor{
                [&](const auto& aPrimitive) { aPrimitive->Submit(aView, aProgram); },
            },
            mesh);
    }
}

void Model::Submit(
    wato::ViewId          aView,
    bgfx::ProgramHandle   aProgram,
    const InstanceBuffer& aBuffer,
    uint64_t              aState)
{
    for (const auto& mesh : mMeshes) {
        aBuffer.SetBuffer();
        bgfx::setState(aState);
        std::visit(
            VariantVisitor{
                [&](const auto& aPrimitive) { aPrimitive->Submit(aView, aProgram); },
            },
            mesh);
    }
}
