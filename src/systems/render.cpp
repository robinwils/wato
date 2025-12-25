#include "systems/render.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/defines.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <span>
#include <unordered_map>
#include <variant>

#include "components/animator.hpp"
#include "components/camera.hpp"
#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/scene_object.hpp"
#include "core/physics/physics.hpp"
#include "core/tile.hpp"
#include "core/types.hpp"
#include "core/window.hpp"
#include "imgui_helper.h"
#include "input/action.hpp"
#include "renderer/instance_buffer.hpp"
#include "renderer/renderer.hpp"
#include "resource/cache.hpp"

void RenderSystem::Execute(Registry& aRegistry, [[maybe_unused]] float aDelta)
{
    auto& renderer = aRegistry.ctx().get<BgfxRenderer&>();
    // This dummy draw call is here to make sure that view 0 is cleared
    // if no other draw calls are submitted to view 0.
    renderer.Touch(0);

    uint64_t state = BGFX_STATE_DEFAULT;

    auto  bpSkinnedShader   = aRegistry.ctx().get<ShaderCache>()["blinnphong_skinned"_hs];
    auto  bpInstancedShader = aRegistry.ctx().get<ShaderCache>()["blinnphong_instanced"_hs];
    auto& modelCache        = aRegistry.ctx().get<ModelCache>();

    // light
    for (auto&& [light, source] : aRegistry.view<const LightSource>().each()) {
        renderer.SetUniform(
            bpSkinnedShader->Uniform("u_lightDir"),
            glm::vec4(source.direction, 0.0f));
        renderer.SetUniform(bpSkinnedShader->Uniform("u_lightCol"), glm::vec4(source.color, 0.0f));
        renderer.SetUniform(
            bpInstancedShader->Uniform("u_lightDir"),
            glm::vec4(source.direction, 0.0f));
        renderer.SetUniform(
            bpInstancedShader->Uniform("u_lightCol"),
            glm::vec4(source.color, 0.0f));
    }

    if (aRegistry.ctx().get<Graph>().GridDirty) {
        updateGridTexture(aRegistry);
    }

    if (!aRegistry.view<const PlacementMode>()->empty()) {
        renderGrid(aRegistry);
    }

    // Collect static meshes for instancing (group by model)
    // Key: model pointer, Value: instance buffer with transforms
    std::unordered_map<Model*, InstanceBuffer> instanceBuffers;

    for (auto&& [entity, obj, t] : aRegistry.view<SceneObject, Transform3D>().each()) {
        auto model = modelCache[obj.ModelHash];
        if (!model) {
            continue;
        }

        // Animated entities are rendered individually
        if (const Animator* animator = aRegistry.try_get<Animator>(entity);
            animator && !animator->FinalBonesMatrices.empty()) {
            uint16_t numBones = static_cast<uint16_t>(animator->FinalBonesMatrices.size());
            if (numBones > 128) {
                numBones = 128;
            }
            renderer.SetUniform(
                bpSkinnedShader->Uniform("u_bones"),
                animator->FinalBonesMatrices[0],
                numBones);
            model->Submit(t.ModelMat(), state);
            continue;
        }

        // Static entities are batched for instancing
        instanceBuffers[model.operator->()].Add(t.ModelMat());
    }

    // Submit instanced batches
    for (auto& [modelPtr, buffer] : instanceBuffers) {
        if (buffer.Empty()) {
            continue;
        }

        if (!buffer.Allocate()) {
            continue;
        }
        modelPtr->Submit(buffer, state);
    }
}

void RenderSystem::updateGridTexture(Registry& aRegistry)
{
    auto& graph    = aRegistry.ctx().get<Graph>();
    auto& renderer = aRegistry.ctx().get<BgfxRenderer&>();

    BX_ASSERT(
        graph.GridLayout().size() == graph.Width() * graph.Height(),
        "incorrect graph data length");

    entt::resource<bgfx::TextureHandle> handle = aRegistry.ctx().get<TextureCache>()["grid_tex"_hs];
    renderer.UpdateTexture2D(
        handle,
        0,
        0,
        0,
        0,
        graph.Width(),
        graph.Height(),
        std::span<const uint8_t>(graph.GridLayout().data(), graph.Width() * graph.Height()));
    graph.GridDirty = false;
}

void RenderSystem::renderGrid(Registry& aRegistry)
{
    if (auto grid = aRegistry.ctx().get<ModelCache>()["grid"_hs]; grid) {
        // WATO_DBG(aRegistry, "grid is {}", *grid);
        // WATO_DBG(aRegistry, "{}", aRegistry.ctx().get<Graph>());
        grid->Submit(glm::identity<glm::mat4>(), BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);
    }
}

void RenderImguiSystem::Execute(Registry& aRegistry, [[maybe_unused]] float aDelta)
{
    auto& window = aRegistry.ctx().get<WatoWindow&>();
    imguiBeginFrame(window.GetInput(), window.Width<int>(), window.Height<int>());
    showImguiDialogs(window.Width<float>(), window.Height<float>());

    for (auto&& [entity, imgui] : aRegistry.view<ImguiDrawable>().each()) {
        auto [camera, transform] = aRegistry.try_get<Camera, Transform3D>(entity);
        ImGui::Text("%s Settings", imgui.Name.c_str());
        if (camera && transform) {
            const auto& [origin, end] = window.MouseUnproject(*camera, transform->Position);

            ImGui::Text("Input Information");
            ImGui::Text("Mouse: %s", glm::to_string(window.GetInput().MouseState.Pos).c_str());
            ImGui::Text("Mouse Ray: %s", glm::to_string(end - origin).c_str());

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
            ImGui::DragFloat3(
                "Light direction",
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
    ImGui::Checkbox("Contact Points", &phy.Params.RenderContactPoints);
    ImGui::Checkbox("Contact Normals", &phy.Params.RenderContactNormals);

    debugRenderer.setIsDebugItemDisplayed(
        rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE,
        phy.Params.RenderShapes);
    debugRenderer.setIsDebugItemDisplayed(
        rp3d::DebugRenderer::DebugItem::COLLIDER_AABB,
        phy.Params.RenderAabb);
    debugRenderer.setIsDebugItemDisplayed(
        rp3d::DebugRenderer::DebugItem::CONTACT_POINT,
        phy.Params.RenderContactPoints);
    debugRenderer.setIsDebugItemDisplayed(
        rp3d::DebugRenderer::DebugItem::CONTACT_NORMAL,
        phy.Params.RenderContactNormals);
#endif

    ImGui::End();

#if WATO_DEBUG
    Camera      cam;
    Transform3D camT;

    for (auto&& [entity, camera, t] : aRegistry.view<Camera, Transform3D>().each()) {
        cam  = camera;
        camT = t;
        break;
    }

    auto& graph = aRegistry.ctx().get<Graph>();
    for (auto&& [entity, imgui, t] : aRegistry.view<ImguiDrawable, Transform3D>().each()) {
        if (imgui.PosOnUnit) {
            GraphCell   c      = GraphCell::FromWorldPoint(t.Position);
            glm::vec3   screen = window.ProjectPosition(t.Position, cam, camT.Position);
            std::string s      = "";

            if (auto* health = aRegistry.try_get<Health>(entity); health) {
                s = fmt::format("{} {}, {} health", c.Location.x, c.Location.y, health->Health);
            } else {
                s = fmt::format("{} {}", c.Location.x, c.Location.y);
            }

            text(
                screen.x,
                window.Height<float>() - screen.y,
                fmt::format("graph_pos_{}", entt::id_type(entity)),
                s);

            if (auto next = graph.GetNextCell(GraphCell::FromWorldPoint(t.Position))) {
                screen = window.ProjectPosition(next->ToWorld(), cam, camT.Position);

                text(
                    screen.x,
                    window.Height<float>() - screen.y - 20.0f,
                    fmt::format("next_graph_pos_{}", entt::id_type(entity)),
                    fmt::format("{} {}", next->Location.x, next->Location.y),
                    imguiRGBA(255, 0, 0));
            }
        }
    }
#endif

    imguiEndFrame();
}

void CameraSystem::Execute(Registry& aRegistry, [[maybe_unused]] float aDelta)
{
    auto& window   = aRegistry.ctx().get<WatoWindow&>();
    auto& renderer = aRegistry.ctx().get<BgfxRenderer&>();
    for (auto&& [entity, camera, transform] : aRegistry.view<Camera, Transform3D>().each()) {
        const auto& viewMat = camera.View(transform.Position);
        const auto& proj    = camera.Projection(window.Width<float>(), window.Height<float>());
        renderer.SetViewTransform(0, viewMat, proj);
        renderer.SetViewRect(0, 0, 0, window.Width<uint16_t>(), window.Height<uint16_t>());

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

void PhysicsDebugSystem::Execute(Registry& aRegistry, [[maybe_unused]] float aDelta)
{
    const auto&                phy           = aRegistry.ctx().get<Physics&>();
    auto&                      renderer      = aRegistry.ctx().get<BgfxRenderer&>();
    rp3d::DebugRenderer const& debugRenderer = phy.World()->getDebugRenderer();

    PosColor::Init();

    auto nTri   = debugRenderer.getNbTriangles();
    auto nLines = debugRenderer.getNbLines();

    auto debugShader = aRegistry.ctx().get<ShaderCache>()["simple"_hs];
    auto debugMat    = std::make_unique<Material>(debugShader);

    if (nTri > 0) {
        auto state = BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_ALWAYS;
        renderer.SubmitDebugGeometry(
            debugRenderer.getTrianglesArray(),
            3 * nTri,
            state,
            debugMat->Program(),
            PosColor::msLayout);
    }
    if (nLines > 0) {
        auto state = BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES;
        renderer.SubmitDebugGeometry(
            debugRenderer.getLinesArray(),
            2 * nLines,
            state,
            debugMat->Program(),
            PosColor::msLayout);
    }
}
#endif
