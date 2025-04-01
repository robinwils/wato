#include "systems/render.hpp"

#include <bgfx/bgfx.h>

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "components/imgui.hpp"
#include "components/scene_object.hpp"
#include "core/cache.hpp"
#include "core/physics.hpp"
#include "core/window.hpp"
#include "imgui_helper.h"

void RenderSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    // This dummy draw call is here to make sure that view 0 is cleared
    // if no other draw calls are submitted to view 0.
    bgfx::touch(0);

    uint64_t state = BGFX_STATE_DEFAULT;

    auto bpShader = WATO_PROGRAM_CACHE["blinnphong"_hs];
    // light
    for (auto&& [light, source] : aRegistry.view<const LightSource>().each()) {
        bgfx::setUniform(bpShader->Uniform("u_lightDir"),
            glm::value_ptr(glm::vec4(source.direction, 0.0f)));
        bgfx::setUniform(bpShader->Uniform("u_lightCol"),
            glm::value_ptr(glm::vec4(source.color, 0.0f)));
    }
    auto check = aRegistry.view<const PlacementMode>();

    for (auto&& [entity, obj, t] : aRegistry.view<SceneObject, Transform3D>().each()) {
        auto model  = glm::identity<glm::mat4>();
        model       = glm::translate(model, t.Position);
        model      *= glm::mat4_cast(t.Orientation);
        model       = glm::scale(model, t.Scale);

        if (check.contains(entity)) {
            // DBG("GOT Placement mode entity!")
        }

        if (auto primitives = WATO_MODEL_CACHE[obj.model_hash]; primitives) {
            for (const auto* p : *primitives) {
                // Set model matrix for rendering.
                bgfx::setTransform(glm::value_ptr(model));

                // kinda awkward place to put that...
                // obj.material.drawImgui();

                // Set render states.
                bgfx::setState(state);
                p->Submit();
            }
        }
    }
}

void RenderImguiSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto& window = aRegistry.ctx().get<WatoWindow&>();
    imguiBeginFrame(window.GetInput(), window.Width<int>(), window.Height<int>());
    showImguiDialogs(window.Width<float>(), window.Height<float>());

    for (auto&& [entity, imgui] : aRegistry.view<ImguiDrawable>().each()) {
        auto [camera, transform] = aRegistry.try_get<Camera, Transform3D>(entity);
        ImGui::Text("%s Settings", imgui.name.c_str());
        if (camera && transform) {
            window.GetInput().DrawImgui(*camera,
                transform->Position,
                window.Width<float>(),
                window.Height<float>());
            ImGui::DragFloat3("Position", glm::value_ptr(transform->Position), 0.1f, 5.0f);
            ImGui::DragFloat3("Direction", glm::value_ptr(camera->Dir), 0.1f, 2.0f);
            ImGui::DragFloat("FoV (Degree)", &camera->Fov, 10.0f, 120.0f);
            ImGui::DragFloat("Speed", &camera->Speed, 0.1f, 0.01f, 5.0f, "%.03f");
            continue;
        }
        if (transform) {
            ImGui::Text("Position = %s", glm::to_string(transform->Position).c_str());
            continue;
        }

        auto* lightSource = aRegistry.try_get<LightSource>(entity);
        if (lightSource) {
            ImGui::DragFloat3("Light direction",
                glm::value_ptr(lightSource->direction),
                0.1f,
                5.0f);
            ImGui::DragFloat3("Light color", glm::value_ptr(lightSource->color), 0.10f, 2.0f);
        }
    }

    auto& phy = aRegistry.ctx().get<Physics&>();

    ImGui::Text("Physics info");
    if (ImGui::Checkbox("Information Logs", &phy.Params.InfoLogs)
        || ImGui::Checkbox("Warning Logs", &phy.Params.WarningLogs)
        || ImGui::Checkbox("Error logs", &phy.Params.ErrorLogs)) {
        phy.InitLogger();
    }

#if WATO_DEBUG
    rp3d::DebugRenderer& debugRenderer = phy.World()->getDebugRenderer();
    auto                 nTri          = debugRenderer.getNbTriangles();
    auto                 nLines        = debugRenderer.getNbLines();

    ImGui::Text("%d debug lines and %d debug triangles", nLines, nTri);
    ImGui::Checkbox("Collider Shapes", &phy.Params.RenderShapes);
    ImGui::Checkbox("Collider AABB", &phy.Params.RenderAabb);

    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE,
        phy.Params.RenderShapes);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB,
        phy.Params.RenderAabb);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT,
        phy.Params.RenderContactPoints);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_NORMAL,
        phy.Params.RenderContactNormals);
#endif

    imguiEndFrame();
}

void CameraSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto& window = aRegistry.ctx().get<WatoWindow&>();
    for (auto&& [entity, camera, transform] : aRegistry.view<Camera, Transform3D>().each()) {
        const auto& viewMat = camera.View(transform.Position);
        const auto& proj    = camera.Projection(window.Width<float>(), window.Height<float>());
        bgfx::setViewTransform(0, glm::value_ptr(viewMat), glm::value_ptr(proj));
        bgfx::setViewRect(0, 0, 0, window.Width<uint16_t>(), window.Height<uint16_t>());

        // just because I know there is only 1 camera (for now)
        // TODO: put in registry context var as singleton ?
        break;
    }
}

#if WATO_DEBUG
static bool vInit = false;
struct PosColor {
    float    X;
    float    Y;
    float    Z;
    uint32_t RGBA;

    static void Init()
    {
        if (vInit) {
            return;
        }
        msLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
        vInit = true;
    }

    static bgfx::VertexLayout msLayout;
};
bgfx::VertexLayout PosColor::msLayout;

void PhysicsDebugSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    const auto&                phy           = aRegistry.ctx().get<Physics&>();
    rp3d::DebugRenderer const& debugRenderer = phy.World()->getDebugRenderer();

    PosColor::Init();

    auto nTri   = debugRenderer.getNbTriangles();
    auto nLines = debugRenderer.getNbLines();

    auto  debugShader = WATO_PROGRAM_CACHE["simple"_hs];
    auto* debugMat    = new Material(debugShader);
    if (nTri > 0) {
        auto state = BGFX_STATE_DEFAULT;
        if (3 * nTri == bgfx::getAvailTransientVertexBuffer(3 * nTri, PosColor::msLayout)) {
            bgfx::TransientVertexBuffer vb{};
            bgfx::allocTransientVertexBuffer(&vb, 3 * nTri, PosColor::msLayout);

            bx::memCopy(vb.data, debugRenderer.getTrianglesArray(), 3 * nTri * sizeof(PosColor));

            bgfx::setState(state);
            bgfx::setVertexBuffer(0, &vb);
            bgfx::submit(0, debugMat->Program(), bgfx::ViewMode::Default);
        }
    }
    if (nLines > 0) {
        auto state = BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES;
        if (2 * nLines == bgfx::getAvailTransientVertexBuffer(2 * nLines, PosColor::msLayout)) {
            bgfx::TransientVertexBuffer vb{};
            bgfx::allocTransientVertexBuffer(&vb, 2 * nLines, PosColor::msLayout);

            bx::memCopy(vb.data, debugRenderer.getLinesArray(), 2 * nLines * sizeof(PosColor));

            bgfx::setState(state);
            bgfx::setVertexBuffer(0, &vb);
            bgfx::submit(0, debugMat->Program(), bgfx::ViewMode::Default);
        }
    }
}
#endif
