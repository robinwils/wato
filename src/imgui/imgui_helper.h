/*
 * Copyright 2011-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#ifndef IMGUI_H_HEADER_GUARD
#define IMGUI_H_HEADER_GUARD

#define IMGUI_USER_CONFIG "wato_imgui_config.h"

#include <IconsFontAwesome6.h>
#include <IconsKenney.h>
#include <bgfx/bgfx.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <input/input.hpp>

#define IMGUI_MBUT_LEFT   0x01
#define IMGUI_MBUT_RIGHT  0x02
#define IMGUI_MBUT_MIDDLE 0x04

inline uint32_t imguiRGBA(uint8_t aRed, uint8_t aGreen, uint8_t aBlue, uint8_t aAlpha = 255)
{
    return 0 | (uint32_t(aRed) << 0) | (uint32_t(aGreen) << 8) | (uint32_t(aBlue) << 16)
           | (uint32_t(aAlpha) << 24);
}

namespace bx
{
struct AllocatorI;
}

void imguiCreate(float aFontSize = 18.0f, bx::AllocatorI* aAllocator = NULL);
void imguiDestroy();

void imguiBeginFrame(const Input& aInput,
    int                           aWidth,
    int                           aHeight,
    int                           aInputChar = -1,
    bgfx::ViewId                  aView      = 255);

void imguiEndFrame();

void showImguiDialogs(float aWidth, float aHeight);
void showStatsDialog(const char* aErrorText = NULL);
void showSettingsDialog(float aWidth, float aHeight);

namespace ImGui
{
#define IMGUI_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

///
inline ImTextureID toId(bgfx::TextureHandle aHandle, uint8_t aFlags, uint8_t aMip)
{
    union {
        struct {
            bgfx::TextureHandle Handle;
            uint8_t             Flags;
            uint8_t             Mip;
        } State;
        ImTextureID ID;
    } tex;
    tex.State.Handle = aHandle;
    tex.State.Flags  = aFlags;
    tex.State.Mip    = aMip;
    return tex.ID;
}

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
inline void Image(bgfx::TextureHandle aHandle,
    uint8_t                           aFlags,
    uint8_t                           aMip,
    const ImVec2&                     aSize,
    const ImVec2&                     aUv0       = ImVec2(0.0f, 0.0f),
    const ImVec2&                     aUv1       = ImVec2(1.0f, 1.0f),
    const ImVec4&                     aTintCol   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
    const ImVec4&                     aBorderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f))
{
    Image(toId(aHandle, aFlags, aMip), aSize, aUv0, aUv1, aTintCol, aBorderCol);
}

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
inline void Image(bgfx::TextureHandle aHandle,
    const ImVec2&                     aSize,
    const ImVec2&                     aUv0       = ImVec2(0.0f, 0.0f),
    const ImVec2&                     aUv1       = ImVec2(1.0f, 1.0f),
    const ImVec4&                     aTintCol   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
    const ImVec4&                     aBorderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f))
{
    Image(aHandle, IMGUI_FLAGS_ALPHA_BLEND, 0, aSize, aUv0, aUv1, aTintCol, aBorderCol);
}

// Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
inline bool ImageButton(bgfx::TextureHandle aHandle,
    uint8_t                                 aFlags,
    uint8_t                                 aMip,
    const ImVec2&                           aSize,
    const ImVec2&                           aUv0     = ImVec2(0.0f, 0.0f),
    const ImVec2&                           aUv1     = ImVec2(1.0f, 1.0f),
    const ImVec4&                           aBgCol   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
    const ImVec4&                           aTintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    return ImageButton("image", toId(aHandle, aFlags, aMip), aSize, aUv0, aUv1, aBgCol, aTintCol);
}

// Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
inline bool ImageButton(bgfx::TextureHandle aHandle,
    const ImVec2&                           aSize,
    const ImVec2&                           aUv0     = ImVec2(0.0f, 0.0f),
    const ImVec2&                           aUv1     = ImVec2(1.0f, 1.0f),
    const ImVec4&                           aBgCol   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
    const ImVec4&                           aTintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    return ImageButton(aHandle, IMGUI_FLAGS_ALPHA_BLEND, 0, aSize, aUv0, aUv1, aBgCol, aTintCol);
}

///
inline void NextLine() { SetCursorPosY(GetCursorPosY() + GetTextLineHeightWithSpacing()); }

///
inline bool MouseOverArea()
{
    return false || ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered()
           || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)
        //		|| ImGuizmo::IsOver()
        ;
}

///
void PushEnabled(bool aEnabled);

///
void PopEnabled();

}  // namespace ImGui

#endif  // IMGUI_H_HEADER_GUARD
