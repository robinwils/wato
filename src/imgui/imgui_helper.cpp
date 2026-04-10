/*
 * Copyright 2014-2015 Daniel Collin. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/allocator.h>
#include <bx/math.h>
#include <bx/timer.h>

#include <renderer/bgfx_utils.hpp>

BX_PRAGMA_DIAGNOSTIC_PUSH();
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wdouble-promotion")
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wold-style-cast")
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wcast-align")
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wimplicit-int-float-conversion")
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wsign-conversion")
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wconversion")

#include <imgui/fs_imgui_image.bin.h>
#include <imgui/fs_ocornut_imgui.bin.h>
#include <imgui/roboto_regular.ttf.h>
#include <imgui/robotomono_regular.ttf.h>
#include <imgui/vs_imgui_image.bin.h>
#include <imgui/vs_ocornut_imgui.bin.h>

#include "iconfonts/IconsFontAwesome4.h_fontawesome-webfont.ttf.h"
#include "iconfonts/IconsKenney.h_kenney-icon-font.ttf.h"
#include "imgui_helper.h"

static const bgfx::EmbeddedShader kEmbeddedShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(vs_imgui_image),
    BGFX_EMBEDDED_SHADER(fs_imgui_image),
    BGFX_EMBEDDED_SHADER_END()};

struct FontRangeMerge {
    const void* Data;
    size_t      Size;
    ImWchar     Ranges[3];
};

static FontRangeMerge sFontRangeMerge[] = {
    {s_kenney_icon_font_ttf, sizeof(s_kenney_icon_font_ttf), {ICON_MIN_KI, ICON_MAX_KI, 0}},
    {s_fontawesome_webfont_ttf, sizeof(s_fontawesome_webfont_ttf), {ICON_MIN_FA, ICON_MAX_FA, 0}},
};

static void* memAlloc(size_t aSize, void* aUserData);
static void  memFree(void* aPtr, void* aUserData);

struct OcornutImguiContext {
    void Render(ImDrawData* aDrawData)
    {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates
        // != framebuffer coordinates)
        int fbWidth  = (int)(aDrawData->DisplaySize.x * aDrawData->FramebufferScale.x);
        int fbHeight = (int)(aDrawData->DisplaySize.y * aDrawData->FramebufferScale.y);
        if (fbWidth <= 0 || fbHeight <= 0) return;

        bgfx::setViewName(ViewID, "ImGui");
        bgfx::setViewMode(ViewID, bgfx::ViewMode::Sequential);

        const bgfx::Caps* caps = bgfx::getCaps();
        {
            float ortho[16];
            float x      = aDrawData->DisplayPos.x;
            float y      = aDrawData->DisplayPos.y;
            float width  = aDrawData->DisplaySize.x;
            float height = aDrawData->DisplaySize.y;

            bx::mtxOrtho(
                ortho,
                x,
                x + width,
                y + height,
                y,
                0.0f,
                1000.0f,
                0.0f,
                caps->homogeneousDepth);
            bgfx::setViewTransform(ViewID, NULL, ortho);
            bgfx::setViewRect(ViewID, 0, 0, uint16_t(width), uint16_t(height));
        }

        const ImVec2 clipPos = aDrawData->DisplayPos;  // (0,0) unless using multi-viewports
        const ImVec2 clipScale =
            aDrawData->FramebufferScale;  // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        for (int32_t ii = 0, num = aDrawData->CmdListsCount; ii < num; ++ii) {
            bgfx::TransientVertexBuffer tvb;
            bgfx::TransientIndexBuffer  tib;

            const ImDrawList* drawList    = aDrawData->CmdLists[ii];
            uint32_t          numVertices = (uint32_t)drawList->VtxBuffer.size();
            uint32_t          numIndices  = (uint32_t)drawList->IdxBuffer.size();

            if (!checkAvailTransientBuffers(numVertices, Layout, numIndices)) {
                // not enough space in transient buffer just quit drawing the rest...
                break;
            }

            bgfx::allocTransientVertexBuffer(&tvb, numVertices, Layout);
            bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

            ImDrawVert* verts = (ImDrawVert*)tvb.data;
            bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

            ImDrawIdx* indices = (ImDrawIdx*)tib.data;
            bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

            bgfx::Encoder* encoder = bgfx::begin();

            for (const ImDrawCmd *cmd    = drawList->CmdBuffer.begin(),
                                 *cmdEnd = drawList->CmdBuffer.end();
                 cmd != cmdEnd;
                 ++cmd) {
                if (cmd->UserCallback) {
                    cmd->UserCallback(drawList, cmd);
                } else if (0 != cmd->ElemCount) {
                    uint64_t state =
                        0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;

                    bgfx::TextureHandle th      = Texture;
                    bgfx::ProgramHandle program = Program;

                    if (0 != cmd->TextureId) {
                        union {
                            ImTextureID Ptr;
                            struct {
                                bgfx::TextureHandle Handle;
                                uint8_t             Flags;
                                uint8_t             Mip;
                            } State;
                        } texture  = {cmd->TextureId};
                        state     |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.State.Flags)
                                         ? BGFX_STATE_BLEND_FUNC(
                                           BGFX_STATE_BLEND_SRC_ALPHA,
                                           BGFX_STATE_BLEND_INV_SRC_ALPHA)
                                         : BGFX_STATE_NONE;
                        th         = texture.State.Handle;
                        if (0 != texture.State.Mip) {
                            const float lodEnabled[4] = {
                                float(texture.State.Mip),
                                1.0f,
                                0.0f,
                                0.0f};
                            bgfx::setUniform(UniformImageLodEnabled, lodEnabled);
                            program = ImageProgram;
                        }
                    } else {
                        state |= BGFX_STATE_BLEND_FUNC(
                            BGFX_STATE_BLEND_SRC_ALPHA,
                            BGFX_STATE_BLEND_INV_SRC_ALPHA);
                    }

                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec4 clipRect;
                    clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
                    clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
                    clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
                    clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

                    if (clipRect.x < fbWidth && clipRect.y < fbHeight && clipRect.z >= 0.0f
                        && clipRect.w >= 0.0f) {
                        const uint16_t xx = uint16_t(bx::max(clipRect.x, 0.0f));
                        const uint16_t yy = uint16_t(bx::max(clipRect.y, 0.0f));
                        encoder->setScissor(
                            xx,
                            yy,
                            uint16_t(bx::min(clipRect.z, 65535.0f) - xx),
                            uint16_t(bx::min(clipRect.w, 65535.0f) - yy));

                        encoder->setState(state);
                        encoder->setTexture(0, UniformTex, th);
                        encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
                        encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
                        encoder->submit(ViewID, program);
                    }
                }
            }

            bgfx::end(encoder);
        }
    }

    void Create(float aFontSize, bx::AllocatorI* aAllocator)
    {
        Allocator = aAllocator;

        if (NULL == aAllocator) {
            static bx::DefaultAllocator allocator;
            Allocator = &allocator;
        }

        ViewID = 255;
        Last   = bx::getHPCounter();

        ImGui::SetAllocatorFunctions(memAlloc, memFree, NULL);

        ImguiCtx = ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DeltaTime   = 1.0f / 60.0f;
        io.IniFilename = NULL;

        SetupStyle(true);

        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

        bgfx::RendererType::Enum type = bgfx::getRendererType();
        Program                       = bgfx::createProgram(
            bgfx::createEmbeddedShader(kEmbeddedShaders, type, "vs_ocornut_imgui"),
            bgfx::createEmbeddedShader(kEmbeddedShaders, type, "fs_ocornut_imgui"),
            true);

        UniformImageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
        ImageProgram           = bgfx::createProgram(
            bgfx::createEmbeddedShader(kEmbeddedShaders, type, "vs_imgui_image"),
            bgfx::createEmbeddedShader(kEmbeddedShaders, type, "fs_imgui_image"),
            true);

        Layout.begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        UniformTex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

        uint8_t* data;
        int32_t  width;
        int32_t  height;
        {
            ImFontConfig config;
            config.FontDataOwnedByAtlas = false;
            config.MergeMode            = false;
            //			config.MergeGlyphCenterV = true;

            const ImWchar* ranges       = io.Fonts->GetGlyphRangesCyrillic();
            Fonts[ImGui::Font::Regular] = io.Fonts->AddFontFromMemoryTTF(
                (void*)s_robotoRegularTtf,
                sizeof(s_robotoRegularTtf),
                aFontSize,
                &config,
                ranges);
            Fonts[ImGui::Font::Mono] = io.Fonts->AddFontFromMemoryTTF(
                (void*)s_robotoMonoRegularTtf,
                sizeof(s_robotoMonoRegularTtf),
                aFontSize - 3.0f,
                &config,
                ranges);

            config.MergeMode = true;
            config.DstFont   = Fonts[ImGui::Font::Regular];

            for (uint32_t ii = 0; ii < BX_COUNTOF(sFontRangeMerge); ++ii) {
                const FontRangeMerge& frm = sFontRangeMerge[ii];

                io.Fonts->AddFontFromMemoryTTF(
                    (void*)frm.Data,
                    (int)frm.Size,
                    aFontSize - 3.0f,
                    &config,
                    frm.Ranges);
            }
        }

        io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

        Texture = bgfx::createTexture2D(
            (uint16_t)width,
            (uint16_t)height,
            false,
            1,
            bgfx::TextureFormat::BGRA8,
            0,
            bgfx::copy(data, width * height * 4));
    }

    void Destroy()
    {
        ImGui::DestroyContext(ImguiCtx);

        bgfx::destroy(UniformTex);
        bgfx::destroy(Texture);

        bgfx::destroy(UniformImageLodEnabled);
        bgfx::destroy(ImageProgram);
        bgfx::destroy(Program);

        Allocator = NULL;
    }

    void SetupStyle(bool aDark)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        if (aDark) {
            ImGui::StyleColorsDark(&style);
        } else {
            ImGui::StyleColorsLight(&style);
        }

        // Sizing and spacing
        style.WindowPadding = ImVec2(10.0f, 10.0f);
        style.FramePadding  = ImVec2(8.0f, 4.0f);
        style.ItemSpacing   = ImVec2(8.0f, 6.0f);
        style.ScrollbarSize = 12.0f;
        style.GrabMinSize   = 10.0f;

        // Sharp, minimal rounding
        style.WindowRounding    = 5.0f;
        style.FrameRounding     = 3.0f;
        style.PopupRounding     = 2.0f;
        style.ScrollbarRounding = 2.0f;
        style.GrabRounding      = 2.0f;
        style.TabRounding       = 2.0f;

        // Subtle borders
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize  = 0.0f;
        style.PopupBorderSize  = 0.0f;

        // Warm dark base with sage-teal accent
        // Base:   #24282E #2C3038 #363C46 #434A56
        // Accent: #5B9EA6 (sage teal), bright: #7BB8C0
        // Text:   #D8DDE3 / #808890 (disabled)
        ImVec4* colors = style.Colors;

        // Text
        colors[ImGuiCol_Text]         = ImGui::ColorU8(216, 221, 227);
        colors[ImGuiCol_TextDisabled] = ImGui::ColorU8(168, 140, 125);

        // Backgrounds
        colors[ImGuiCol_WindowBg] = ImGui::ColorU8(36, 40, 46);
        colors[ImGuiCol_ChildBg]  = ImGui::ColorU8(36, 40, 46, 0);
        colors[ImGuiCol_PopupBg]  = ImGui::ColorU8(40, 44, 52, 245);

        // Borders
        colors[ImGuiCol_Border]       = ImGui::ColorU8(60, 66, 76);
        colors[ImGuiCol_BorderShadow] = ImGui::ColorU8(0, 0, 0, 0);

        // Frames (input fields, checkboxes) — lifted from bg for contrast
        colors[ImGuiCol_FrameBg]        = ImGui::ColorU8(46, 68, 71);
        colors[ImGuiCol_FrameBgHovered] = ImGui::ColorU8(54, 60, 70);
        colors[ImGuiCol_FrameBgActive]  = ImGui::ColorU8(67, 74, 86);

        // Title bars
        colors[ImGuiCol_TitleBg]          = ImGui::ColorU8(46, 68, 71);
        colors[ImGuiCol_TitleBgActive]    = ImGui::ColorU8(72, 132, 140);
        colors[ImGuiCol_TitleBgCollapsed] = ImGui::ColorU8(28, 32, 38);

        // Menu
        colors[ImGuiCol_MenuBarBg] = ImGui::ColorU8(32, 36, 42);

        // Scrollbars
        colors[ImGuiCol_ScrollbarBg]          = ImGui::ColorU8(36, 40, 46);
        colors[ImGuiCol_ScrollbarGrab]        = ImGui::ColorU8(60, 66, 76);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImGui::ColorU8(76, 82, 94);
        colors[ImGuiCol_ScrollbarGrabActive]  = ImGui::ColorU8(91, 158, 166);

        // Buttons — clear contrast against background
        colors[ImGuiCol_Button]        = ImGui::ColorU8(54, 60, 70);
        colors[ImGuiCol_ButtonHovered] = ImGui::ColorU8(91, 158, 166);
        colors[ImGuiCol_ButtonActive]  = ImGui::ColorU8(72, 132, 140);

        // Headers (collapsing headers, selectable)
        colors[ImGuiCol_Header]        = ImGui::ColorU8(44, 48, 56);
        colors[ImGuiCol_HeaderHovered] = ImGui::ColorU8(60, 66, 76);
        colors[ImGuiCol_HeaderActive]  = ImGui::ColorU8(72, 78, 90);

        // Tabs (1.91.9 naming)
        colors[ImGuiCol_Tab]                       = ImGui::ColorU8(36, 40, 46);
        colors[ImGuiCol_TabHovered]                = ImGui::ColorU8(91, 158, 166);
        colors[ImGuiCol_TabSelected]               = ImGui::ColorU8(50, 56, 64);
        colors[ImGuiCol_TabSelectedOverline]       = ImGui::ColorU8(91, 158, 166);
        colors[ImGuiCol_TabDimmed]                 = ImGui::ColorU8(32, 36, 42);
        colors[ImGuiCol_TabDimmedSelected]         = ImGui::ColorU8(40, 44, 52);
        colors[ImGuiCol_TabDimmedSelectedOverline] = ImGui::ColorU8(60, 66, 76);

        // Interactive elements
        colors[ImGuiCol_CheckMark]        = ImGui::ColorU8(123, 184, 192);
        colors[ImGuiCol_SliderGrab]       = ImGui::ColorU8(91, 158, 166);
        colors[ImGuiCol_SliderGrabActive] = ImGui::ColorU8(123, 184, 192);

        // Separators
        colors[ImGuiCol_Separator]        = ImGui::ColorU8(60, 66, 76);
        colors[ImGuiCol_SeparatorHovered] = ImGui::ColorU8(91, 158, 166);
        colors[ImGuiCol_SeparatorActive]  = ImGui::ColorU8(123, 184, 192);

        // Resize grip
        colors[ImGuiCol_ResizeGrip]        = ImGui::ColorU8(60, 66, 76, 80);
        colors[ImGuiCol_ResizeGripHovered] = ImGui::ColorU8(91, 158, 166, 170);
        colors[ImGuiCol_ResizeGripActive]  = ImGui::ColorU8(91, 158, 166, 240);

        // Plots
        colors[ImGuiCol_PlotLines]            = ImGui::ColorU8(91, 158, 166);
        colors[ImGuiCol_PlotLinesHovered]     = ImGui::ColorU8(123, 184, 192);
        colors[ImGuiCol_PlotHistogram]        = ImGui::ColorU8(91, 158, 166);
        colors[ImGuiCol_PlotHistogramHovered] = ImGui::ColorU8(123, 184, 192);

        // Tables
        colors[ImGuiCol_TableHeaderBg]     = ImGui::ColorU8(40, 44, 52);
        colors[ImGuiCol_TableBorderStrong] = ImGui::ColorU8(60, 66, 76);
        colors[ImGuiCol_TableBorderLight]  = ImGui::ColorU8(50, 56, 64);
        colors[ImGuiCol_TableRowBg]        = ImGui::ColorU8(0, 0, 0, 0);
        colors[ImGuiCol_TableRowBgAlt]     = ImGui::ColorU8(255, 255, 255, 8);

        // Misc
        colors[ImGuiCol_TextLink]              = ImGui::ColorU8(123, 184, 192);
        colors[ImGuiCol_TextSelectedBg]        = ImGui::ColorU8(91, 158, 166, 100);
        colors[ImGuiCol_DragDropTarget]        = ImGui::ColorU8(91, 158, 166);
        colors[ImGuiCol_NavCursor]             = ImGui::ColorU8(91, 158, 166);
        colors[ImGuiCol_NavWindowingHighlight] = ImGui::ColorU8(255, 255, 255, 178);
        colors[ImGuiCol_NavWindowingDimBg]     = ImGui::ColorU8(200, 200, 200, 50);
        colors[ImGuiCol_ModalWindowDimBg]      = ImGui::ColorU8(0, 0, 0, 140);
    }

    void BeginFrame(const Input& aInput, int aWidth, int aHeight, bgfx::ViewId aViewId)
    {
        ViewID = aViewId;

        ImGuiIO& io = ImGui::GetIO();

        // Add all queued input characters
        for (uint32_t c : aInput.InputChars()) {
            io.AddInputCharacter(c);
        }

        io.DisplaySize = ImVec2((float)aWidth, (float)aHeight);

        const int64_t now       = bx::getHPCounter();
        const int64_t frameTime = now - Last;
        Last                    = now;
        const double freq       = double(bx::getHPFrequency());
        io.DeltaTime            = float(frameTime / freq);

        io.AddMousePosEvent((float)aInput.MouseState.Pos.x, (float)aInput.MouseState.Pos.y);

        io.AddMouseButtonEvent(
            ImGuiMouseButton_Left,
            aInput.MouseState.IsKeyPressed(Mouse::Button::Left));
        io.AddMouseButtonEvent(
            ImGuiMouseButton_Right,
            aInput.MouseState.IsKeyPressed(Mouse::Button::Right));
        io.AddMouseButtonEvent(
            ImGuiMouseButton_Middle,
            aInput.MouseState.IsKeyPressed(Mouse::Button::Middle));
        io.AddMouseWheelEvent(
            static_cast<float>(aInput.MouseState.Scroll.x),
            static_cast<float>(aInput.MouseState.Scroll.y));

        // Modifier keys
        const bool ctrl = aInput.KeyboardState.IsKeyPressed(Keyboard::LeftControl)
                          || aInput.KeyboardState.IsKeyPressed(Keyboard::RightControl);
        const bool shift = aInput.KeyboardState.IsKeyPressed(Keyboard::LeftShift)
                           || aInput.KeyboardState.IsKeyPressed(Keyboard::RightShift);
        const bool alt = aInput.KeyboardState.IsKeyPressed(Keyboard::LeftAlt)
                         || aInput.KeyboardState.IsKeyPressed(Keyboard::RightAlt);
        const bool super = aInput.KeyboardState.IsKeyPressed(Keyboard::LeftSuper)
                           || aInput.KeyboardState.IsKeyPressed(Keyboard::RightSuper);

        io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
        io.AddKeyEvent(ImGuiMod_Shift, shift);
        io.AddKeyEvent(ImGuiMod_Alt, alt);
        io.AddKeyEvent(ImGuiMod_Super, super);

#define ADD_KEY(imKey, key) io.AddKeyEvent(imKey, aInput.KeyboardState.IsKeyPressed(key))
        ADD_KEY(ImGuiKey_Tab, Keyboard::Tab);
        ADD_KEY(ImGuiKey_LeftArrow, Keyboard::Left);
        ADD_KEY(ImGuiKey_RightArrow, Keyboard::Right);
        ADD_KEY(ImGuiKey_UpArrow, Keyboard::Up);
        ADD_KEY(ImGuiKey_DownArrow, Keyboard::Down);
        ADD_KEY(ImGuiKey_PageUp, Keyboard::PageUp);
        ADD_KEY(ImGuiKey_PageDown, Keyboard::PageDown);
        ADD_KEY(ImGuiKey_Home, Keyboard::Home);
        ADD_KEY(ImGuiKey_End, Keyboard::End);
        ADD_KEY(ImGuiKey_Insert, Keyboard::Insert);
        ADD_KEY(ImGuiKey_Delete, Keyboard::Delete);
        ADD_KEY(ImGuiKey_Backspace, Keyboard::Backspace);
        ADD_KEY(ImGuiKey_Space, Keyboard::Space);
        ADD_KEY(ImGuiKey_Enter, Keyboard::Enter);
        ADD_KEY(ImGuiKey_Escape, Keyboard::Escape);
        ADD_KEY(ImGuiKey_KeypadEnter, Keyboard::KpEnter);

        // Letters for shortcuts (Ctrl+A, Ctrl+C, Ctrl+V, Ctrl+X, Ctrl+Y, Ctrl+Z)
        ADD_KEY(ImGuiKey_A, Keyboard::A);
        ADD_KEY(ImGuiKey_C, Keyboard::C);
        ADD_KEY(ImGuiKey_V, Keyboard::V);
        ADD_KEY(ImGuiKey_X, Keyboard::X);
        ADD_KEY(ImGuiKey_Y, Keyboard::Y);
        ADD_KEY(ImGuiKey_Z, Keyboard::Z);
#undef ADD_KEY

        ImGui::NewFrame();
    }

    void EndFrame()
    {
        ImGui::Render();
        Render(ImGui::GetDrawData());
    }

    ImGuiContext*       ImguiCtx;
    bx::AllocatorI*     Allocator;
    bgfx::VertexLayout  Layout;
    bgfx::ProgramHandle Program;
    bgfx::ProgramHandle ImageProgram;
    bgfx::TextureHandle Texture;
    bgfx::UniformHandle UniformTex;
    bgfx::UniformHandle UniformImageLodEnabled;
    ImFont*             Fonts[ImGui::Font::Count];
    int64_t             Last;
    bgfx::ViewId        ViewID;
#if USE_ENTRY
    ImGuiKey m_keyMap[(int)entry::Key::Count];
#endif  // USE_ENTRY
};

static OcornutImguiContext sCtx;

static void* memAlloc(size_t aSize, void* aUserData)
{
    BX_UNUSED(aUserData);
    return bx::alloc(sCtx.Allocator, aSize);
}

static void memFree(void* aPtr, void* aUserData)
{
    BX_UNUSED(aUserData);
    bx::free(sCtx.Allocator, aPtr);
}

void imguiCreate(float aFontSize, bx::AllocatorI* aAllocator)
{
    sCtx.Create(aFontSize, aAllocator);
}

void imguiDestroy() { sCtx.Destroy(); }

void imguiBeginFrame(const Input& aInput, int aWidth, int aHeight, bgfx::ViewId aViewId)
{
    sCtx.BeginFrame(aInput, aWidth, aHeight, aViewId);
}

void imguiEndFrame() { sCtx.EndFrame(); }
struct SampleData {
    static constexpr uint32_t kNumSamples = 100;

    SampleData() { Reset(); }

    void Reset()
    {
        Offset = 0;
        bx::memSet(Values, 0, sizeof(Values));

        Min = 0.0f;
        Max = 0.0f;
        Avg = 0.0f;
    }

    void PushSample(float aValue)
    {
        Values[Offset] = aValue;
        Offset         = (Offset + 1) % kNumSamples;

        float min = bx::max<float>();
        float max = bx::min<float>();
        float avg = 0.0f;

        for (uint32_t ii = 0; ii < kNumSamples; ++ii) {
            const float val  = Values[ii];
            min              = bx::min(min, val);
            max              = bx::max(max, val);
            avg             += val;
        }

        Min = min;
        Max = max;
        Avg = avg / kNumSamples;
    }

    int32_t Offset;
    float   Values[kNumSamples];

    float Min;
    float Max;
    float Avg;
};
static bool       sShowStats = false;
static SampleData sFrameTime;

static bool bar(float aWidth, float aMaxWidth, float aHeight, const ImVec4& aColor)
{
    const ImGuiStyle& style = ImGui::GetStyle();

    ImVec4 hoveredColor(
        aColor.x + aColor.x * 0.1f,
        aColor.y + aColor.y * 0.1f,
        aColor.z + aColor.z * 0.1f,
        aColor.w + aColor.w * 0.1f);

    ImGui::PushStyleColor(ImGuiCol_Button, aColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, aColor);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, style.ItemSpacing.y));

    bool itemHovered = false;

    ImGui::Button("##", ImVec2(aWidth, aHeight));
    itemHovered |= ImGui::IsItemHovered();

    ImGui::SameLine();
    ImGui::InvisibleButton("##", ImVec2(bx::max(1.0f, aMaxWidth - aWidth), aHeight));
    itemHovered |= ImGui::IsItemHovered();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    return itemHovered;
}

static const ImVec4 kResourceColor(0.5f, 0.5f, 0.5f, 1.0f);

static void resourceBar(
    const char* aName,
    const char* aTooltip,
    uint32_t    aNum,
    uint32_t    aMax,
    float       aMaxWidth,
    float       aHeight)
{
    bool itemHovered = false;

    ImGui::Text("%s: %4d / %4d", aName, aNum, aMax);
    itemHovered |= ImGui::IsItemHovered();
    ImGui::SameLine();

    const float percentage = float(aNum) / float(aMax);

    itemHovered |= bar(bx::max(1.0f, percentage * aMaxWidth), aMaxWidth, aHeight, kResourceColor);
    ImGui::SameLine();

    ImGui::Text("%5.2f%%", percentage * 100.0f);

    if (itemHovered) {
        ImGui::SetTooltip("%s %5.2f%%", aTooltip, percentage * 100.0f);
    }
}

void text(float aX, float aY, const std::string& aName, const std::string& aText, uint32_t aColor)
{
    ImGui::SetNextWindowPos(ImVec2(aX, aY));
    ImGui::Begin(
        aName.c_str(),
        nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoInputs);
    ImGui::PushStyleColor(ImGuiCol_Text, aColor);
    ImGui::Text("%s", aText.c_str());
    ImGui::PopStyleColor();
    ImGui::End();
}

void showImguiDialogs(float aWidth, float aHeight)
{
    showStatsDialog();
    showSettingsDialog(aWidth, aHeight);
}

void showSettingsDialog(float aWidth, float aHeight)
{
    ImGui::SetNextWindowPos(ImVec2(aWidth - aWidth / 3.0f - 10.0f, 50.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(aWidth / 3.0f, aHeight / 3.5f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Settings", NULL, 0);
    // ImGui::ShowStyleEditor();
}

void showStatsDialog(const char* aErrorText)
{
    ImGui::SetNextWindowPos(ImVec2(10.0f, 50.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.0f, 210.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("wato");

    ImGui::Separator();

    if (NULL != aErrorText) {
        const int64_t now  = bx::getHPCounter();
        const int64_t freq = bx::getHPFrequency();
        const float   time = float(now % freq) / float(freq);

        bool blink = time > 0.5f;

        ImGui::PushStyleColor(
            ImGuiCol_Text,
            blink ? ImVec4(1.0, 0.0, 0.0, 1.0) : ImVec4(1.0, 1.0, 1.0, 1.0));
        ImGui::TextWrapped("%s", aErrorText);
        ImGui::Separator();
        ImGui::PopStyleColor();
    }

    {
        const bgfx::Caps* caps = bgfx::getCaps();
        if (0 != (caps->supported & BGFX_CAPS_GRAPHICS_DEBUGGER)) {
            ImGui::SameLine();
            ImGui::Text(ICON_FA_SNOWFLAKE);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 3.0f));

        ImGui::SameLine();
        sShowStats ^= ImGui::Button(ICON_FA_CHART_SIMPLE);

        ImGui::PopStyleVar();
    }

    const bgfx::Stats* stats   = bgfx::getStats();
    const double       toMsCpu = 1000.0 / stats->cpuTimerFreq;
    const double       toMsGpu = 1000.0 / stats->gpuTimerFreq;
    const double       frameMs = double(stats->cpuTimeFrame) * toMsCpu;

    sFrameTime.PushSample(float(frameMs));

    char frameTextOverlay[256];
    bx::snprintf(
        frameTextOverlay,
        BX_COUNTOF(frameTextOverlay),
        "%s%.3fms, %s%.3fms\nAvg: %.3fms, %.1f FPS",
        ICON_FA_ARROW_DOWN,
        sFrameTime.Min,
        ICON_FA_ARROW_UP,
        sFrameTime.Max,
        sFrameTime.Avg,
        1000.0f / sFrameTime.Avg);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImColor(0.0f, 0.5f, 0.15f, 1.0f).Value);
    ImGui::PlotHistogram(
        "Frame",
        sFrameTime.Values,
        SampleData::kNumSamples,
        sFrameTime.Offset,
        frameTextOverlay,
        0.0f,
        60.0f,
        ImVec2(0.0f, 45.0f));
    ImGui::PopStyleColor();

    ImGui::Text(
        "Submit CPU %0.3f, GPU %0.3f (L: %d)",
        double(stats->cpuTimeEnd - stats->cpuTimeBegin) * toMsCpu,
        double(stats->gpuTimeEnd - stats->gpuTimeBegin) * toMsGpu,
        stats->maxGpuLatency);

    if (-INT64_MAX != stats->gpuMemoryUsed) {
        char tmp0[64];
        bx::prettify(tmp0, BX_COUNTOF(tmp0), stats->gpuMemoryUsed);

        char tmp1[64];
        bx::prettify(tmp1, BX_COUNTOF(tmp1), stats->gpuMemoryMax);

        ImGui::Text("GPU mem: %s / %s", tmp0, tmp1);
    }

    if (sShowStats) {
        ImGui::SetNextWindowSize(ImVec2(300.0f, 500.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin(ICON_FA_CHART_SIMPLE " Stats", &sShowStats)) {
            if (ImGui::CollapsingHeader(ICON_FA_PUZZLE_PIECE " Resources")) {
                const bgfx::Caps* caps = bgfx::getCaps();

                const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
                const float maxWidth   = 90.0f;

                ImGui::PushFont(ImGui::Font::Mono);
                ImGui::Text("Res: Num  / Max");
                resourceBar(
                    "DIB",
                    "Dynamic index buffers",
                    stats->numDynamicIndexBuffers,
                    caps->limits.maxDynamicIndexBuffers,
                    maxWidth,
                    itemHeight);
                resourceBar(
                    "DVB",
                    "Dynamic vertex buffers",
                    stats->numDynamicVertexBuffers,
                    caps->limits.maxDynamicVertexBuffers,
                    maxWidth,
                    itemHeight);
                resourceBar(
                    " FB",
                    "Frame buffers",
                    stats->numFrameBuffers,
                    caps->limits.maxFrameBuffers,
                    maxWidth,
                    itemHeight);
                resourceBar(
                    " IB",
                    "Index buffers",
                    stats->numIndexBuffers,
                    caps->limits.maxIndexBuffers,
                    maxWidth,
                    itemHeight);
                resourceBar(
                    " OQ",
                    "Occlusion queries",
                    stats->numOcclusionQueries,
                    caps->limits.maxOcclusionQueries,
                    maxWidth,
                    itemHeight);
                resourceBar(
                    "  P",
                    "Programs",
                    stats->numPrograms,
                    caps->limits.maxPrograms,
                    maxWidth,
                    itemHeight);
                resourceBar(
                    "  S",
                    "Shaders",
                    stats->numShaders,
                    caps->limits.maxShaders,
                    maxWidth,
                    itemHeight);
                resourceBar(
                    "  T",
                    "Textures",
                    stats->numTextures,
                    caps->limits.maxTextures,
                    maxWidth,
                    itemHeight);
                resourceBar(
                    "  U",
                    "Uniforms",
                    stats->numUniforms,
                    caps->limits.maxUniforms,
                    maxWidth,
                    itemHeight);
                resourceBar(
                    " VB",
                    "Vertex buffers",
                    stats->numVertexBuffers,
                    caps->limits.maxVertexBuffers,
                    maxWidth,
                    itemHeight);
                resourceBar(
                    " VL",
                    "Vertex layouts",
                    stats->numVertexLayouts,
                    caps->limits.maxVertexLayouts,
                    maxWidth,
                    itemHeight);
                ImGui::PopFont();
            }

            if (ImGui::CollapsingHeader(ICON_FA_CLOCK " Profiler")) {
                if (0 == stats->numViews) {
                    ImGui::Text("Profiler is not enabled.");
                } else {
                    if (ImGui::BeginChild("##view_profiler", ImVec2(0.0f, 0.0f))) {
                        ImGui::PushFont(ImGui::Font::Mono);

                        ImVec4 cpuColor(0.5f, 1.0f, 0.5f, 1.0f);
                        ImVec4 gpuColor(0.5f, 0.5f, 1.0f, 1.0f);

                        const float  itemHeight            = ImGui::GetTextLineHeightWithSpacing();
                        const float  itemHeightWithSpacing = ImGui::GetFrameHeightWithSpacing();
                        const double toCpuMs               = 1000.0 / double(stats->cpuTimerFreq);
                        const double toGpuMs               = 1000.0 / double(stats->gpuTimerFreq);
                        const float  scale                 = 3.0f;

                        if (ImGui::BeginListBox(
                                "Encoders",
                                ImVec2(
                                    ImGui::GetWindowWidth(),
                                    stats->numEncoders * itemHeightWithSpacing))) {
                            ImGuiListClipper clipper;
                            clipper.Begin(stats->numEncoders, itemHeight);

                            while (clipper.Step()) {
                                for (int32_t pos = clipper.DisplayStart; pos < clipper.DisplayEnd;
                                     ++pos) {
                                    const bgfx::EncoderStats& encoderStats =
                                        stats->encoderStats[pos];

                                    ImGui::Text("%3d", pos);
                                    ImGui::SameLine(64.0f);

                                    const float maxWidth = 30.0f * scale;
                                    const float cpuMs    = float(
                                        (encoderStats.cpuTimeEnd - encoderStats.cpuTimeBegin)
                                        * toCpuMs);
                                    const float cpuWidth = bx::clamp(cpuMs * scale, 1.0f, maxWidth);

                                    if (bar(cpuWidth, maxWidth, itemHeight, cpuColor)) {
                                        ImGui::SetTooltip("Encoder %d, CPU: %f [ms]", pos, cpuMs);
                                    }
                                }
                            }

                            ImGui::EndListBox();
                        }

                        ImGui::Separator();

                        if (ImGui::BeginListBox(
                                "Views",
                                ImVec2(
                                    ImGui::GetWindowWidth(),
                                    stats->numViews * itemHeightWithSpacing))) {
                            ImGuiListClipper clipper;
                            clipper.Begin(stats->numViews, itemHeight);

                            while (clipper.Step()) {
                                for (int32_t pos = clipper.DisplayStart; pos < clipper.DisplayEnd;
                                     ++pos) {
                                    const bgfx::ViewStats& viewStats = stats->viewStats[pos];

                                    ImGui::Text("%3d %3d %s", pos, viewStats.view, viewStats.name);

                                    const float maxWidth       = 30.0f * scale;
                                    const float cpuTimeElapsed = float(
                                        (viewStats.cpuTimeEnd - viewStats.cpuTimeBegin) * toCpuMs);
                                    const float gpuTimeElapsed = float(
                                        (viewStats.gpuTimeEnd - viewStats.gpuTimeBegin) * toGpuMs);
                                    const float cpuWidth =
                                        bx::clamp(cpuTimeElapsed * scale, 1.0f, maxWidth);
                                    const float gpuWidth =
                                        bx::clamp(gpuTimeElapsed * scale, 1.0f, maxWidth);

                                    ImGui::SameLine(64.0f);

                                    if (bar(cpuWidth, maxWidth, itemHeight, cpuColor)) {
                                        ImGui::SetTooltip(
                                            "View %d \"%s\", CPU: %f [ms]",
                                            pos,
                                            viewStats.name,
                                            cpuTimeElapsed);
                                    }

                                    ImGui::SameLine();
                                    if (bar(gpuWidth, maxWidth, itemHeight, gpuColor)) {
                                        ImGui::SetTooltip(
                                            "View: %d \"%s\", GPU: %f [ms]",
                                            pos,
                                            viewStats.name,
                                            gpuTimeElapsed);
                                    }
                                }
                            }

                            ImGui::EndListBox();
                        }

                        ImGui::PopFont();
                    }

                    ImGui::EndChild();
                }
            }
        }
        ImGui::End();
    }
    ImGui::End();
}

namespace ImGui
{
void PushFont(Font::Enum aFont) { PushFont(sCtx.Fonts[aFont]); }

void PushEnabled(bool aEnabled)
{
    extern void PushItemFlag(int aOption, bool aEnabled);
    PushItemFlag(ImGuiItemFlags_Disabled, !aEnabled);
    PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * (aEnabled ? 1.0f : 0.5f));
}

void PopEnabled()
{
    extern void PopItemFlag();
    PopItemFlag();
    PopStyleVar();
}

ImString::ImString() : mPtr(NULL) {}

ImString::ImString(const ImString& aRhs) : mPtr(NULL)
{
    if (NULL != aRhs.mPtr && 0 != strcmp(aRhs.mPtr, "")) {
        mPtr = ImStrdup(aRhs.mPtr);
    }
}

ImString::ImString(const char* aRhs) : mPtr(NULL)
{
    if (NULL != aRhs && 0 != strcmp(aRhs, "")) {
        mPtr = ImStrdup(aRhs);
    }
}

ImString::~ImString() { Clear(); }

ImString& ImString::operator=(const ImString& aRhs)
{
    if (this != &aRhs) {
        *this = aRhs.mPtr;
    }

    return *this;
}

ImString& ImString::operator=(const char* aRhs)
{
    if (mPtr != aRhs) {
        Clear();

        if (NULL != aRhs && 0 != strcmp(aRhs, "")) {
            mPtr = ImStrdup(aRhs);
        }
    }

    return *this;
}

void ImString::Clear()
{
    if (NULL != mPtr) {
        MemFree(mPtr);
        mPtr = NULL;
    }
}

bool ImString::IsEmpty() const { return NULL == mPtr; }
}  // namespace ImGui
BX_PRAGMA_DIAGNOSTIC_POP();
