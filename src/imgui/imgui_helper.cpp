/*
 * Copyright 2014-2015 Daniel Collin. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "imgui_helper.h"

#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/allocator.h>
#include <bx/math.h>
#include <bx/timer.h>
#include <imgui/fs_imgui_image.bin.h>
#include <imgui/fs_ocornut_imgui.bin.h>
#include <imgui/roboto_regular.ttf.h>
#include <imgui/robotomono_regular.ttf.h>
#include <imgui/vs_imgui_image.bin.h>
#include <imgui/vs_ocornut_imgui.bin.h>

#include <renderer/bgfx_utils.hpp>

#include "iconfonts/IconsFontAwesome4.h_fontawesome-webfont.ttf.h"
#include "iconfonts/IconsKenney.h_kenney-icon-font.ttf.h"

static const bgfx::EmbeddedShader EMBEDDED_SHADERS[] = {
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
    {s_kenney_icon_font_ttf,    sizeof(s_kenney_icon_font_ttf),    {ICON_MIN_KI, ICON_MAX_KI, 0}},
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

            bx::mtxOrtho(ortho,
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
                                         ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,
                                           BGFX_STATE_BLEND_INV_SRC_ALPHA)
                                         : BGFX_STATE_NONE;
                        th         = texture.State.Handle;
                        if (0 != texture.State.Mip) {
                            const float lodEnabled[4] = {float(texture.State.Mip),
                                1.0f,
                                0.0f,
                                0.0f};
                            bgfx::setUniform(UniformImageLodEnabled, lodEnabled);
                            program = ImageProgram;
                        }
                    } else {
                        state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,
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
                        encoder->setScissor(xx,
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

        ViewID     = 255;
        LastScroll = 0;
        Last       = bx::getHPCounter();

        ImGui::SetAllocatorFunctions(memAlloc, memFree, NULL);

        ImguiCtx = ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DeltaTime   = 1.0f / 60.0f;
        io.IniFilename = NULL;

        SetupStyle(true);

        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

#if USE_ENTRY
        for (int32_t ii = 0; ii < (int32_t)entry::Key::Count; ++ii) {
            m_keyMap[ii] = ImGuiKey_None;
        }

        m_keyMap[entry::Key::Esc]          = ImGuiKey_Escape;
        m_keyMap[entry::Key::Return]       = ImGuiKey_Enter;
        m_keyMap[entry::Key::Tab]          = ImGuiKey_Tab;
        m_keyMap[entry::Key::Space]        = ImGuiKey_Space;
        m_keyMap[entry::Key::Backspace]    = ImGuiKey_Backspace;
        m_keyMap[entry::Key::Up]           = ImGuiKey_UpArrow;
        m_keyMap[entry::Key::Down]         = ImGuiKey_DownArrow;
        m_keyMap[entry::Key::Left]         = ImGuiKey_LeftArrow;
        m_keyMap[entry::Key::Right]        = ImGuiKey_RightArrow;
        m_keyMap[entry::Key::Insert]       = ImGuiKey_Insert;
        m_keyMap[entry::Key::Delete]       = ImGuiKey_Delete;
        m_keyMap[entry::Key::Home]         = ImGuiKey_Home;
        m_keyMap[entry::Key::End]          = ImGuiKey_End;
        m_keyMap[entry::Key::PageUp]       = ImGuiKey_PageUp;
        m_keyMap[entry::Key::PageDown]     = ImGuiKey_PageDown;
        m_keyMap[entry::Key::Print]        = ImGuiKey_PrintScreen;
        m_keyMap[entry::Key::Plus]         = ImGuiKey_Equal;
        m_keyMap[entry::Key::Minus]        = ImGuiKey_Minus;
        m_keyMap[entry::Key::LeftBracket]  = ImGuiKey_LeftBracket;
        m_keyMap[entry::Key::RightBracket] = ImGuiKey_RightBracket;
        m_keyMap[entry::Key::Semicolon]    = ImGuiKey_Semicolon;
        m_keyMap[entry::Key::Quote]        = ImGuiKey_Apostrophe;
        m_keyMap[entry::Key::Comma]        = ImGuiKey_Comma;
        m_keyMap[entry::Key::Period]       = ImGuiKey_Period;
        m_keyMap[entry::Key::Slash]        = ImGuiKey_Slash;
        m_keyMap[entry::Key::Backslash]    = ImGuiKey_Backslash;
        m_keyMap[entry::Key::Tilde]        = ImGuiKey_GraveAccent;
        m_keyMap[entry::Key::F1]           = ImGuiKey_F1;
        m_keyMap[entry::Key::F2]           = ImGuiKey_F2;
        m_keyMap[entry::Key::F3]           = ImGuiKey_F3;
        m_keyMap[entry::Key::F4]           = ImGuiKey_F4;
        m_keyMap[entry::Key::F5]           = ImGuiKey_F5;
        m_keyMap[entry::Key::F6]           = ImGuiKey_F6;
        m_keyMap[entry::Key::F7]           = ImGuiKey_F7;
        m_keyMap[entry::Key::F8]           = ImGuiKey_F8;
        m_keyMap[entry::Key::F9]           = ImGuiKey_F9;
        m_keyMap[entry::Key::F10]          = ImGuiKey_F10;
        m_keyMap[entry::Key::F11]          = ImGuiKey_F11;
        m_keyMap[entry::Key::F12]          = ImGuiKey_F12;
        m_keyMap[entry::Key::NumPad0]      = ImGuiKey_Keypad0;
        m_keyMap[entry::Key::NumPad1]      = ImGuiKey_Keypad1;
        m_keyMap[entry::Key::NumPad2]      = ImGuiKey_Keypad2;
        m_keyMap[entry::Key::NumPad3]      = ImGuiKey_Keypad3;
        m_keyMap[entry::Key::NumPad4]      = ImGuiKey_Keypad4;
        m_keyMap[entry::Key::NumPad5]      = ImGuiKey_Keypad5;
        m_keyMap[entry::Key::NumPad6]      = ImGuiKey_Keypad6;
        m_keyMap[entry::Key::NumPad7]      = ImGuiKey_Keypad7;
        m_keyMap[entry::Key::NumPad8]      = ImGuiKey_Keypad8;
        m_keyMap[entry::Key::NumPad9]      = ImGuiKey_Keypad9;
        m_keyMap[entry::Key::Key0]         = ImGuiKey_0;
        m_keyMap[entry::Key::Key1]         = ImGuiKey_1;
        m_keyMap[entry::Key::Key2]         = ImGuiKey_2;
        m_keyMap[entry::Key::Key3]         = ImGuiKey_3;
        m_keyMap[entry::Key::Key4]         = ImGuiKey_4;
        m_keyMap[entry::Key::Key5]         = ImGuiKey_5;
        m_keyMap[entry::Key::Key6]         = ImGuiKey_6;
        m_keyMap[entry::Key::Key7]         = ImGuiKey_7;
        m_keyMap[entry::Key::Key8]         = ImGuiKey_8;
        m_keyMap[entry::Key::Key9]         = ImGuiKey_9;
        m_keyMap[entry::Key::KeyA]         = ImGuiKey_A;
        m_keyMap[entry::Key::KeyB]         = ImGuiKey_B;
        m_keyMap[entry::Key::KeyC]         = ImGuiKey_C;
        m_keyMap[entry::Key::KeyD]         = ImGuiKey_D;
        m_keyMap[entry::Key::KeyE]         = ImGuiKey_E;
        m_keyMap[entry::Key::KeyF]         = ImGuiKey_F;
        m_keyMap[entry::Key::KeyG]         = ImGuiKey_G;
        m_keyMap[entry::Key::KeyH]         = ImGuiKey_H;
        m_keyMap[entry::Key::KeyI]         = ImGuiKey_I;
        m_keyMap[entry::Key::KeyJ]         = ImGuiKey_J;
        m_keyMap[entry::Key::KeyK]         = ImGuiKey_K;
        m_keyMap[entry::Key::KeyL]         = ImGuiKey_L;
        m_keyMap[entry::Key::KeyM]         = ImGuiKey_M;
        m_keyMap[entry::Key::KeyN]         = ImGuiKey_N;
        m_keyMap[entry::Key::KeyO]         = ImGuiKey_O;
        m_keyMap[entry::Key::KeyP]         = ImGuiKey_P;
        m_keyMap[entry::Key::KeyQ]         = ImGuiKey_Q;
        m_keyMap[entry::Key::KeyR]         = ImGuiKey_R;
        m_keyMap[entry::Key::KeyS]         = ImGuiKey_S;
        m_keyMap[entry::Key::KeyT]         = ImGuiKey_T;
        m_keyMap[entry::Key::KeyU]         = ImGuiKey_U;
        m_keyMap[entry::Key::KeyV]         = ImGuiKey_V;
        m_keyMap[entry::Key::KeyW]         = ImGuiKey_W;
        m_keyMap[entry::Key::KeyX]         = ImGuiKey_X;
        m_keyMap[entry::Key::KeyY]         = ImGuiKey_Y;
        m_keyMap[entry::Key::KeyZ]         = ImGuiKey_Z;

        io.ConfigFlags |=
            0 | ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard;

        m_keyMap[entry::Key::GamepadStart]     = ImGuiKey_GamepadStart;
        m_keyMap[entry::Key::GamepadBack]      = ImGuiKey_GamepadBack;
        m_keyMap[entry::Key::GamepadY]         = ImGuiKey_GamepadFaceUp;
        m_keyMap[entry::Key::GamepadA]         = ImGuiKey_GamepadFaceDown;
        m_keyMap[entry::Key::GamepadX]         = ImGuiKey_GamepadFaceLeft;
        m_keyMap[entry::Key::GamepadB]         = ImGuiKey_GamepadFaceRight;
        m_keyMap[entry::Key::GamepadUp]        = ImGuiKey_GamepadDpadUp;
        m_keyMap[entry::Key::GamepadDown]      = ImGuiKey_GamepadDpadDown;
        m_keyMap[entry::Key::GamepadLeft]      = ImGuiKey_GamepadDpadLeft;
        m_keyMap[entry::Key::GamepadRight]     = ImGuiKey_GamepadDpadRight;
        m_keyMap[entry::Key::GamepadShoulderL] = ImGuiKey_GamepadL1;
        m_keyMap[entry::Key::GamepadShoulderR] = ImGuiKey_GamepadR1;
        m_keyMap[entry::Key::GamepadThumbL]    = ImGuiKey_GamepadL3;
        m_keyMap[entry::Key::GamepadThumbR]    = ImGuiKey_GamepadR3;
#endif  // USE_ENTRY

        bgfx::RendererType::Enum type = bgfx::getRendererType();
        Program                       = bgfx::createProgram(
            bgfx::createEmbeddedShader(EMBEDDED_SHADERS, type, "vs_ocornut_imgui"),
            bgfx::createEmbeddedShader(EMBEDDED_SHADERS, type, "fs_ocornut_imgui"),
            true);

        UniformImageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
        ImageProgram           = bgfx::createProgram(
            bgfx::createEmbeddedShader(EMBEDDED_SHADERS, type, "vs_imgui_image"),
            bgfx::createEmbeddedShader(EMBEDDED_SHADERS, type, "fs_imgui_image"),
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
            Fonts[ImGui::Font::Regular] = io.Fonts->AddFontFromMemoryTTF((void*)s_robotoRegularTtf,
                sizeof(s_robotoRegularTtf),
                aFontSize,
                &config,
                ranges);
            Fonts[ImGui::Font::Mono] = io.Fonts->AddFontFromMemoryTTF((void*)s_robotoMonoRegularTtf,
                sizeof(s_robotoMonoRegularTtf),
                aFontSize - 3.0f,
                &config,
                ranges);

            config.MergeMode = true;
            config.DstFont   = Fonts[ImGui::Font::Regular];

            for (uint32_t ii = 0; ii < BX_COUNTOF(sFontRangeMerge); ++ii) {
                const FontRangeMerge& frm = sFontRangeMerge[ii];

                io.Fonts->AddFontFromMemoryTTF((void*)frm.Data,
                    (int)frm.Size,
                    aFontSize - 3.0f,
                    &config,
                    frm.Ranges);
            }
        }

        io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

        Texture = bgfx::createTexture2D((uint16_t)width,
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
        // Doug Binks' darl color scheme
        // https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9
        ImGuiStyle& style = ImGui::GetStyle();
        if (aDark) {
            ImGui::StyleColorsDark(&style);
        } else {
            ImGui::StyleColorsLight(&style);
        }

        style.FrameRounding    = 4.0f;
        style.WindowBorderSize = 0.0f;
    }

    void BeginFrame(const Input& aInput,
        int                      aWidth,
        int                      aHeight,
        int                      aInputChar,
        bgfx::ViewId             aViewId)
    {
        ViewID = aViewId;

        ImGuiIO& io = ImGui::GetIO();
        if (aInputChar >= 0) {
            io.AddInputCharacter(aInputChar);
        }

        io.DisplaySize = ImVec2((float)aWidth, (float)aHeight);

        const int64_t now       = bx::getHPCounter();
        const int64_t frameTime = now - Last;
        Last                    = now;
        const double freq       = double(bx::getHPFrequency());
        io.DeltaTime            = float(frameTime / freq);

        io.AddMousePosEvent((float)aInput.MouseState.Pos.x, (float)aInput.MouseState.Pos.y);

        io.AddMouseButtonEvent(ImGuiMouseButton_Left,
            aInput.MouseState.IsKeyPressed(Mouse::Button::Left));
        io.AddMouseButtonEvent(ImGuiMouseButton_Right,
            aInput.MouseState.IsKeyPressed(Mouse::Button::Right));
        io.AddMouseButtonEvent(ImGuiMouseButton_Middle,
            aInput.MouseState.IsKeyPressed(Mouse::Button::Middle));
        io.AddMouseWheelEvent(0.0f, (float)(aInput.MouseState.Scroll.y - LastScroll));
        LastScroll = aInput.MouseState.Scroll.y;

        ImGui::NewFrame();
    }

    void EndFrame()
    {
        ImGui::End();
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
    double              LastScroll;
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

void imguiBeginFrame(const Input& aInput,
    int                           aWidth,
    int                           aHeight,
    int                           aInputChar,
    bgfx::ViewId                  aViewId)
{
    sCtx.BeginFrame(aInput, aWidth, aHeight, aInputChar, aViewId);
}

void imguiEndFrame() { sCtx.EndFrame(); }
struct SampleData {
    static constexpr uint32_t NUM_SAMPLES = 100;

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
        Offset         = (Offset + 1) % NUM_SAMPLES;

        float min = bx::max<float>();
        float max = bx::min<float>();
        float avg = 0.0f;

        for (uint32_t ii = 0; ii < NUM_SAMPLES; ++ii) {
            const float val  = Values[ii];
            min              = bx::min(min, val);
            max              = bx::max(max, val);
            avg             += val;
        }

        Min = min;
        Max = max;
        Avg = avg / NUM_SAMPLES;
    }

    int32_t Offset;
    float   Values[NUM_SAMPLES];

    float Min;
    float Max;
    float Avg;
};
static bool       sShowStats = false;
static SampleData sFrameTime;

static bool bar(float aWidth, float aMaxWidth, float aHeight, const ImVec4& aColor)
{
    const ImGuiStyle& style = ImGui::GetStyle();

    ImVec4 hoveredColor(aColor.x + aColor.x * 0.1f,
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

static const ImVec4 RESOURCE_COLOR(0.5f, 0.5f, 0.5f, 1.0f);

static void resourceBar(const char* aName,
    const char*                     aTooltip,
    uint32_t                        aNum,
    uint32_t                        aMax,
    float                           aMaxWidth,
    float                           aHeight)
{
    bool itemHovered = false;

    ImGui::Text("%s: %4d / %4d", aName, aNum, aMax);
    itemHovered |= ImGui::IsItemHovered();
    ImGui::SameLine();

    const float percentage = float(aNum) / float(aMax);

    itemHovered |= bar(bx::max(1.0f, percentage * aMaxWidth), aMaxWidth, aHeight, RESOURCE_COLOR);
    ImGui::SameLine();

    ImGui::Text("%5.2f%%", percentage * 100.0f);

    if (itemHovered) {
        ImGui::SetTooltip("%s %5.2f%%", aTooltip, percentage * 100.0f);
    }
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

        ImGui::PushStyleColor(ImGuiCol_Text,
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
    bx::snprintf(frameTextOverlay,
        BX_COUNTOF(frameTextOverlay),
        "%s%.3fms, %s%.3fms\nAvg: %.3fms, %.1f FPS",
        ICON_FA_ARROW_DOWN,
        sFrameTime.Min,
        ICON_FA_ARROW_UP,
        sFrameTime.Max,
        sFrameTime.Avg,
        1000.0f / sFrameTime.Avg);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImColor(0.0f, 0.5f, 0.15f, 1.0f).Value);
    ImGui::PlotHistogram("Frame",
        sFrameTime.Values,
        SampleData::NUM_SAMPLES,
        sFrameTime.Offset,
        frameTextOverlay,
        0.0f,
        60.0f,
        ImVec2(0.0f, 45.0f));
    ImGui::PopStyleColor();

    ImGui::Text("Submit CPU %0.3f, GPU %0.3f (L: %d)",
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
                resourceBar("DIB",
                    "Dynamic index buffers",
                    stats->numDynamicIndexBuffers,
                    caps->limits.maxDynamicIndexBuffers,
                    maxWidth,
                    itemHeight);
                resourceBar("DVB",
                    "Dynamic vertex buffers",
                    stats->numDynamicVertexBuffers,
                    caps->limits.maxDynamicVertexBuffers,
                    maxWidth,
                    itemHeight);
                resourceBar(" FB",
                    "Frame buffers",
                    stats->numFrameBuffers,
                    caps->limits.maxFrameBuffers,
                    maxWidth,
                    itemHeight);
                resourceBar(" IB",
                    "Index buffers",
                    stats->numIndexBuffers,
                    caps->limits.maxIndexBuffers,
                    maxWidth,
                    itemHeight);
                resourceBar(" OQ",
                    "Occlusion queries",
                    stats->numOcclusionQueries,
                    caps->limits.maxOcclusionQueries,
                    maxWidth,
                    itemHeight);
                resourceBar("  P",
                    "Programs",
                    stats->numPrograms,
                    caps->limits.maxPrograms,
                    maxWidth,
                    itemHeight);
                resourceBar("  S",
                    "Shaders",
                    stats->numShaders,
                    caps->limits.maxShaders,
                    maxWidth,
                    itemHeight);
                resourceBar("  T",
                    "Textures",
                    stats->numTextures,
                    caps->limits.maxTextures,
                    maxWidth,
                    itemHeight);
                resourceBar("  U",
                    "Uniforms",
                    stats->numUniforms,
                    caps->limits.maxUniforms,
                    maxWidth,
                    itemHeight);
                resourceBar(" VB",
                    "Vertex buffers",
                    stats->numVertexBuffers,
                    caps->limits.maxVertexBuffers,
                    maxWidth,
                    itemHeight);
                resourceBar(" VL",
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

                        if (ImGui::BeginListBox("Encoders",
                                ImVec2(ImGui::GetWindowWidth(),
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
                                    const float cpuMs =
                                        float((encoderStats.cpuTimeEnd - encoderStats.cpuTimeBegin)
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

                        if (ImGui::BeginListBox("Views",
                                ImVec2(ImGui::GetWindowWidth(),
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
                                        ImGui::SetTooltip("View %d \"%s\", CPU: %f [ms]",
                                            pos,
                                            viewStats.name,
                                            cpuTimeElapsed);
                                    }

                                    ImGui::SameLine();
                                    if (bar(gpuWidth, maxWidth, itemHeight, gpuColor)) {
                                        ImGui::SetTooltip("View: %d \"%s\", GPU: %f [ms]",
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

BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(
    4505);  // error C4505: '' : unreferenced local function has been removed
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC(
    "-Wunused-function");  // warning: 'int rect_width_compare(const void*, const
                           // void*)' defined but not used
BX_PRAGMA_DIAGNOSTIC_PUSH();
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG("-Wunknown-pragmas")
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC(
    "-Wtype-limits");  // warning: comparison is always true due to limited range of data type
/*#define STBTT_malloc(_size, _userData) memAlloc(_size, _userData)
#define STBTT_free(_ptr, _userData) memFree(_ptr, _userData)
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>*/
BX_PRAGMA_DIAGNOSTIC_POP();
