/*
 * Copyright 2014-2015 Daniel Collin. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/allocator.h>
#include <bx/math.h>
#include <bx/timer.h>

#include "imgui_helper.h"

#include <renderer/bgfx_utils.hpp>

#include <imgui/vs_ocornut_imgui.bin.h>
#include <imgui/fs_ocornut_imgui.bin.h>
#include <imgui/vs_imgui_image.bin.h>
#include <imgui/fs_imgui_image.bin.h>

#include <imgui/roboto_regular.ttf.h>
#include <imgui/robotomono_regular.ttf.h>
#include <IconsKenney.h_kenney-icon-font.ttf.h>
#include <IconsFontAwesome4.h_fontawesome-webfont.ttf.h>
#include <core/camera.hpp>

static const bgfx::EmbeddedShader s_embeddedShaders[] =
{
	BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
	BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
	BGFX_EMBEDDED_SHADER(vs_imgui_image),
	BGFX_EMBEDDED_SHADER(fs_imgui_image),

	BGFX_EMBEDDED_SHADER_END()
};

struct FontRangeMerge
{
	const void* data;
	size_t      size;
	ImWchar     ranges[3];
};

static FontRangeMerge s_fontRangeMerge[] =
{
	{ s_kenney_icon_font_ttf,      sizeof(s_kenney_icon_font_ttf),      { ICON_MIN_KI, ICON_MAX_KI, 0 } },
	{ s_fontawesome_webfont_ttf, sizeof(s_fontawesome_webfont_ttf), { ICON_MIN_FA, ICON_MAX_FA, 0 } },
};

static void* memAlloc(size_t _size, void* _userData);
static void memFree(void* _ptr, void* _userData);

struct OcornutImguiContext
{
	void render(ImDrawData* _drawData)
	{
		// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
		int32_t dispWidth = int32_t(_drawData->DisplaySize.x * _drawData->FramebufferScale.x);
		int32_t dispHeight = int32_t(_drawData->DisplaySize.y * _drawData->FramebufferScale.y);
		if (dispWidth <= 0
			|| dispHeight <= 0)
		{
			return;
		}

		bgfx::setViewName(m_viewId, "ImGui");
		bgfx::setViewMode(m_viewId, bgfx::ViewMode::Sequential);

		const bgfx::Caps* caps = bgfx::getCaps();
		{
			float ortho[16];
			float x = _drawData->DisplayPos.x;
			float y = _drawData->DisplayPos.y;
			float width = _drawData->DisplaySize.x;
			float height = _drawData->DisplaySize.y;

			bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
			bgfx::setViewTransform(m_viewId, NULL, ortho);
			bgfx::setViewRect(m_viewId, 0, 0, uint16_t(width), uint16_t(height));
		}

		const ImVec2 clipPos = _drawData->DisplayPos;       // (0,0) unless using multi-viewports
		const ImVec2 clipScale = _drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		// Render command lists
		for (int32_t ii = 0, num = _drawData->CmdListsCount; ii < num; ++ii)
		{
			bgfx::TransientVertexBuffer tvb;
			bgfx::TransientIndexBuffer tib;

			const ImDrawList* drawList = _drawData->CmdLists[ii];
			uint32_t numVertices = (uint32_t)drawList->VtxBuffer.size();
			uint32_t numIndices = (uint32_t)drawList->IdxBuffer.size();

			if (!checkAvailTransientBuffers(numVertices, m_layout, numIndices))
			{
				// not enough space in transient buffer just quit drawing the rest...
				break;
			}

			bgfx::allocTransientVertexBuffer(&tvb, numVertices, m_layout);
			bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

			ImDrawVert* verts = (ImDrawVert*)tvb.data;
			bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

			ImDrawIdx* indices = (ImDrawIdx*)tib.data;
			bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

			bgfx::Encoder* encoder = bgfx::begin();

			for (const ImDrawCmd* cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd)
			{
				if (cmd->UserCallback)
				{
					cmd->UserCallback(drawList, cmd);
				}
				else if (0 != cmd->ElemCount)
				{
					uint64_t state = 0
						| BGFX_STATE_WRITE_RGB
						| BGFX_STATE_WRITE_A
						| BGFX_STATE_MSAA
						| BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

					bgfx::TextureHandle th = m_texture;
					bgfx::ProgramHandle program = m_program;					

					// Project scissor/clipping rectangles into framebuffer space
					ImVec4 clipRect;
					clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
					clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
					clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
					clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

					if (clipRect.x < dispWidth
						&& clipRect.y < dispHeight
						&& clipRect.z >= 0.0f
						&& clipRect.w >= 0.0f)
					{
						const uint16_t xx = uint16_t(bx::max(clipRect.x, 0.0f));
						const uint16_t yy = uint16_t(bx::max(clipRect.y, 0.0f));
						encoder->setScissor(xx, yy
							, uint16_t(bx::min(clipRect.z, 65535.0f) - xx)
							, uint16_t(bx::min(clipRect.w, 65535.0f) - yy)
						);

						encoder->setState(state);
						encoder->setTexture(0, s_tex, th);
						encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
						encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
						encoder->submit(m_viewId, program);
					}
				}
			}

			bgfx::end(encoder);
		}
	}

	void create(float _fontSize, bx::AllocatorI* _allocator)
	{
		IMGUI_CHECKVERSION();

		m_allocator = _allocator;

		if (NULL == _allocator)
		{
			static bx::DefaultAllocator allocator;
			m_allocator = &allocator;
		}

		m_viewId = 255;
		m_lastScroll = 0;
		m_last = bx::getHPCounter();

		ImGui::SetAllocatorFunctions(memAlloc, memFree, NULL);

		m_imgui = ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();

		io.DisplaySize = ImVec2(1280.0f, 720.0f);
		io.DeltaTime = 1.0f / 60.0f;
		io.IniFilename = NULL;

		setupStyle(true, 1.0);
		
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

		bgfx::RendererType::Enum type = bgfx::getRendererType();
		m_program = bgfx::createProgram(
			bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui")
			, bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui")
			, true
		);

		u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
		m_imageProgram = bgfx::createProgram(
			bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image")
			, bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image")
			, true
		);

		m_layout
			.begin()
			.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();

		s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

		uint8_t* data;
		int32_t width;
		int32_t height;
		{
			ImFontConfig config;
			config.FontDataOwnedByAtlas = false;
			config.MergeMode = false;
			//			config.MergeGlyphCenterV = true;

			const ImWchar* ranges = io.Fonts->GetGlyphRangesCyrillic();
			m_font[ImGui::Font::Regular] = io.Fonts->AddFontFromMemoryTTF((void*)s_robotoRegularTtf, sizeof(s_robotoRegularTtf), _fontSize, &config, ranges);
			m_font[ImGui::Font::Mono] = io.Fonts->AddFontFromMemoryTTF((void*)s_robotoMonoRegularTtf, sizeof(s_robotoMonoRegularTtf), _fontSize - 3.0f, &config, ranges);

			config.MergeMode = true;
			config.DstFont = m_font[ImGui::Font::Regular];

			for (uint32_t ii = 0; ii < BX_COUNTOF(s_fontRangeMerge); ++ii)
			{
				const FontRangeMerge& frm = s_fontRangeMerge[ii];

				io.Fonts->AddFontFromMemoryTTF((void*)frm.data
					, (int)frm.size
					, _fontSize - 3.0f
					, &config
					, frm.ranges
				);
			}
		}

		io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

		m_texture = bgfx::createTexture2D(
			(uint16_t)width
			, (uint16_t)height
			, false
			, 1
			, bgfx::TextureFormat::BGRA8
			, 0
			, bgfx::copy(data, width * height * 4)
		);
	}

	void destroy()
	{
		ImGui::DestroyContext(m_imgui);

		bgfx::destroy(s_tex);
		bgfx::destroy(m_texture);

		bgfx::destroy(u_imageLodEnabled);
		bgfx::destroy(m_imageProgram);
		bgfx::destroy(m_program);

		m_allocator = NULL;
	}

	void setupStyle(bool _dark, float alpha_)
	{
		// Doug Binks' darl color scheme
		// https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9
		ImGuiStyle& style = ImGui::GetStyle();

		// light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
		style.Alpha = 1.0f;
		style.FrameRounding = 3.0f;
		style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

		if (_dark)
		{
			for (int i = 0; i <= ImGuiCol_COUNT; i++)
			{
				ImVec4& col = style.Colors[i];
				float H, S, V;
				ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

				if (S < 0.1f)
				{
					V = 1.0f - V;
				}
				ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
				if (col.w < 1.00f)
				{
					col.w *= alpha_;
				}
			}
		}
		else
		{
			for (int i = 0; i <= ImGuiCol_COUNT; i++)
			{
				ImVec4& col = style.Colors[i];
				if (col.w < 1.00f)
				{
					col.x *= alpha_;
					col.y *= alpha_;
					col.z *= alpha_;
					col.w *= alpha_;
				}
			}
		}
	}

	void beginFrame(
		  Input _input
		, int _width
		, int _height
		, int _inputChar
		, bgfx::ViewId _viewId
	)
	{
		m_viewId = _viewId;

		ImGuiIO& io = ImGui::GetIO();
		if (_inputChar >= 0)
		{
			io.AddInputCharacter(_inputChar);
		}

		io.DisplaySize = ImVec2((float)_width, (float)_height);

		const int64_t now = bx::getHPCounter();
		const int64_t frameTime = now - m_last;
		m_last = now;
		const double freq = double(bx::getHPFrequency());
		io.DeltaTime = float(frameTime / freq);

		io.AddMousePosEvent( (float)_input.mouseState.pos.x, (float)_input.mouseState.pos.y);

		io.AddMouseButtonEvent(ImGuiMouseButton_Left, _input.isMouseButtonPressed(Mouse::Button::Left));
		io.AddMouseButtonEvent(ImGuiMouseButton_Right, _input.isMouseButtonPressed(Mouse::Button::Right));
		io.AddMouseButtonEvent(ImGuiMouseButton_Middle, _input.isMouseButtonPressed(Mouse::Button::Middle));
		io.AddMouseWheelEvent(0.0f, (float)(_input.mouseState.scroll.y  - m_lastScroll) );
		m_lastScroll = _input.mouseState.scroll.y;

		ImGui::NewFrame();
	}

	void endFrame()
	{
		ImGui::Render();
		render(ImGui::GetDrawData());
	}

	ImGuiContext* m_imgui;
	bx::AllocatorI* m_allocator;
	bgfx::VertexLayout  m_layout;
	bgfx::ProgramHandle m_program;
	bgfx::ProgramHandle m_imageProgram;
	bgfx::TextureHandle m_texture;
	bgfx::UniformHandle s_tex;
	bgfx::UniformHandle u_imageLodEnabled;
	ImFont* m_font[ImGui::Font::Count];
	int64_t m_last;
	int32_t m_lastScroll;
	bgfx::ViewId m_viewId;
#if USE_ENTRY
	ImGuiKey m_keyMap[(int)entry::Key::Count];
#endif // USE_ENTRY
};

static OcornutImguiContext s_ctx;

static void* memAlloc(size_t _size, void* _userData)
{
	BX_UNUSED(_userData);
	return bx::alloc(s_ctx.m_allocator, _size);
}

static void memFree(void* _ptr, void* _userData)
{
	BX_UNUSED(_userData);
	bx::free(s_ctx.m_allocator, _ptr);
}

void imguiCreate(float _fontSize, bx::AllocatorI* _allocator)
{
	s_ctx.create(_fontSize, _allocator);
}

void imguiDestroy()
{
	s_ctx.destroy();
}

void imguiBeginFrame(Input _input, uint16_t _width, uint16_t _height, int _inputChar, bgfx::ViewId _viewId)
{
	s_ctx.beginFrame(_input, _width, _height, _inputChar, _viewId);
}

void imguiEndFrame()
{
	s_ctx.endFrame();
}

struct SampleData
{
	static constexpr uint32_t kNumSamples = 100;

	SampleData()
	{
		reset();
	}

	void reset()
	{
		m_offset = 0;
		bx::memSet(m_values, 0, sizeof(m_values));

		m_min = 0.0f;
		m_max = 0.0f;
		m_avg = 0.0f;
	}

	void pushSample(float value)
	{
		m_values[m_offset] = value;
		m_offset = (m_offset + 1) % kNumSamples;

		float min = bx::max<float>();
		float max = bx::min<float>();
		float avg = 0.0f;

		for (uint32_t ii = 0; ii < kNumSamples; ++ii)
		{
			const float val = m_values[ii];
			min = bx::min(min, val);
			max = bx::max(max, val);
			avg += val;
		}

		m_min = min;
		m_max = max;
		m_avg = avg / kNumSamples;
	}

	int32_t m_offset;
	float m_values[kNumSamples];

	float m_min;
	float m_max;
	float m_avg;
};
static bool s_showStats = false;
static SampleData s_frameTime;

static bool bar(float _width, float _maxWidth, float _height, const ImVec4& _color)
{
	const ImGuiStyle& style = ImGui::GetStyle();

	ImVec4 hoveredColor(
		_color.x + _color.x * 0.1f
		, _color.y + _color.y * 0.1f
		, _color.z + _color.z * 0.1f
		, _color.w + _color.w * 0.1f
	);

	ImGui::PushStyleColor(ImGuiCol_Button, _color);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, _color);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, style.ItemSpacing.y));

	bool itemHovered = false;

	ImGui::Button("##", ImVec2(_width, _height));
	itemHovered |= ImGui::IsItemHovered();

	ImGui::SameLine();
	ImGui::InvisibleButton("##", ImVec2(bx::max(1.0f, _maxWidth - _width), _height));
	itemHovered |= ImGui::IsItemHovered();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(3);

	return itemHovered;
}


static const ImVec4 s_resourceColor(0.5f, 0.5f, 0.5f, 1.0f);

static void resourceBar(const char* _name, const char* _tooltip, uint32_t _num, uint32_t _max, float _maxWidth, float _height)
{
	bool itemHovered = false;

	ImGui::Text("%s: %4d / %4d", _name, _num, _max);
	itemHovered |= ImGui::IsItemHovered();
	ImGui::SameLine();

	const float percentage = float(_num) / float(_max);

	itemHovered |= bar(bx::max(1.0f, percentage * _maxWidth), _maxWidth, _height, s_resourceColor);
	ImGui::SameLine();

	ImGui::Text("%5.2f%%", percentage * 100.0f);

	if (itemHovered)
	{
		ImGui::SetTooltip("%s %5.2f%%"
			, _tooltip
			, percentage * 100.0f
		);
	}
}

void showImguiDialogs(Camera& _cam, const Input& _input, float _width, float _height)
{
	showStatsDialog(_input);
	showSettingsDialog(_cam, _width, _height);
}

void showSettingsDialog(Camera& _cam, float _width, float _height)
{
	ImGui::SetNextWindowPos(
		ImVec2(_width - _width / 5.0f - 10.0f, 10.0f)
		, ImGuiCond_FirstUseEver
	);
	ImGui::SetNextWindowSize(
		ImVec2(_width / 5.0f, _height / 3.5f)
		, ImGuiCond_FirstUseEver
	);
	ImGui::Begin("Settings"
		, NULL
		, 0
	);

	_cam.drawImgui();
	ImGui::End();
}


void showStatsDialog(const Input& _input, const char* _errorText)
{
	char temp[1024];
	bx::snprintf(temp, BX_COUNTOF(temp), "Stats");

	ImGui::SetNextWindowPos(
		ImVec2(10.0f, 50.0f)
		, ImGuiCond_FirstUseEver
	);
	ImGui::SetNextWindowSize(
		ImVec2(300.0f, 210.0f)
		, ImGuiCond_FirstUseEver
	);

	ImGui::Begin(temp);

	ImGui::Separator();

	if (NULL != _errorText)
	{
		const int64_t now = bx::getHPCounter();
		const int64_t freq = bx::getHPFrequency();
		const float   time = float(now % freq) / float(freq);

		bool blink = time > 0.5f;

		ImGui::PushStyleColor(ImGuiCol_Text
			, blink
			? ImVec4(1.0, 0.0, 0.0, 1.0)
			: ImVec4(1.0, 1.0, 1.0, 1.0)
		);
		ImGui::TextWrapped("%s", _errorText);
		ImGui::Separator();
		ImGui::PopStyleColor();
	}

	ImGui::Text("Renderer: %s", bgfx::getRendererName(bgfx::getRendererType()));

	const bgfx::Stats* stats = bgfx::getStats();
	const double toMsCpu = 1000.0 / stats->cpuTimerFreq;
	const double toMsGpu = 1000.0 / stats->gpuTimerFreq;
	const double frameMs = double(stats->cpuTimeFrame) * toMsCpu;

	s_frameTime.pushSample(float(frameMs));

	char frameTextOverlay[256];
	bx::snprintf(frameTextOverlay, BX_COUNTOF(frameTextOverlay), "%s%.3fms, %s%.3fms\nAvg: %.3fms, %.1f FPS"
		, ICON_FA_ARROW_DOWN
		, s_frameTime.m_min
		, ICON_FA_ARROW_UP
		, s_frameTime.m_max
		, s_frameTime.m_avg
		, 1000.0f / s_frameTime.m_avg
	);

	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImColor(0.0f, 0.5f, 0.15f, 1.0f).Value);
	ImGui::PlotHistogram("Frame"
		, s_frameTime.m_values
		, SampleData::kNumSamples
		, s_frameTime.m_offset
		, frameTextOverlay
		, 0.0f
		, 60.0f
		, ImVec2(0.0f, 45.0f)
	);
	ImGui::PopStyleColor();

	ImGui::Text("Submit CPU %0.3f, GPU %0.3f (L: %d)"
		, double(stats->cpuTimeEnd - stats->cpuTimeBegin) * toMsCpu
		, double(stats->gpuTimeEnd - stats->gpuTimeBegin) * toMsGpu
		, stats->maxGpuLatency
	);

	if (-INT64_MAX != stats->gpuMemoryUsed)
	{
		char tmp0[64];
		bx::prettify(tmp0, BX_COUNTOF(tmp0), stats->gpuMemoryUsed);

		char tmp1[64];
		bx::prettify(tmp1, BX_COUNTOF(tmp1), stats->gpuMemoryMax);

		ImGui::Text("GPU mem: %s / %s", tmp0, tmp1);
	}

	if (s_showStats)
	{
		ImGui::SetNextWindowSize(
			ImVec2(300.0f, 500.0f)
			, ImGuiCond_FirstUseEver
		);

		if (ImGui::Begin(ICON_FA_CHART_BAR " Stats", &s_showStats))
		{
			if (ImGui::CollapsingHeader(ICON_FA_PUZZLE_PIECE " Resources"))
			{
				const bgfx::Caps* caps = bgfx::getCaps();

				const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
				const float maxWidth = 90.0f;

				ImGui::PushFont(ImGui::Font::Mono);
				ImGui::Text("Res: Num  / Max");
				resourceBar("DIB", "Dynamic index buffers", stats->numDynamicIndexBuffers, caps->limits.maxDynamicIndexBuffers, maxWidth, itemHeight);
				resourceBar("DVB", "Dynamic vertex buffers", stats->numDynamicVertexBuffers, caps->limits.maxDynamicVertexBuffers, maxWidth, itemHeight);
				resourceBar(" FB", "Frame buffers", stats->numFrameBuffers, caps->limits.maxFrameBuffers, maxWidth, itemHeight);
				resourceBar(" IB", "Index buffers", stats->numIndexBuffers, caps->limits.maxIndexBuffers, maxWidth, itemHeight);
				resourceBar(" OQ", "Occlusion queries", stats->numOcclusionQueries, caps->limits.maxOcclusionQueries, maxWidth, itemHeight);
				resourceBar("  P", "Programs", stats->numPrograms, caps->limits.maxPrograms, maxWidth, itemHeight);
				resourceBar("  S", "Shaders", stats->numShaders, caps->limits.maxShaders, maxWidth, itemHeight);
				resourceBar("  T", "Textures", stats->numTextures, caps->limits.maxTextures, maxWidth, itemHeight);
				resourceBar("  U", "Uniforms", stats->numUniforms, caps->limits.maxUniforms, maxWidth, itemHeight);
				resourceBar(" VB", "Vertex buffers", stats->numVertexBuffers, caps->limits.maxVertexBuffers, maxWidth, itemHeight);
				resourceBar(" VL", "Vertex layouts", stats->numVertexLayouts, caps->limits.maxVertexLayouts, maxWidth, itemHeight);
				ImGui::PopFont();
			}

			if (ImGui::CollapsingHeader(ICON_FA_CLOCK " Profiler"))
			{
				if (0 == stats->numViews)
				{
					ImGui::Text("Profiler is not enabled.");
				}
				else
				{
					if (ImGui::BeginChild("##view_profiler", ImVec2(0.0f, 0.0f)))
					{
						ImGui::PushFont(ImGui::Font::Mono);

						ImVec4 cpuColor(0.5f, 1.0f, 0.5f, 1.0f);
						ImVec4 gpuColor(0.5f, 0.5f, 1.0f, 1.0f);

						const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
						const float itemHeightWithSpacing = ImGui::GetFrameHeightWithSpacing();
						const double toCpuMs = 1000.0 / double(stats->cpuTimerFreq);
						const double toGpuMs = 1000.0 / double(stats->gpuTimerFreq);
						const float  scale = 3.0f;

						if (ImGui::BeginListBox("Encoders", ImVec2(ImGui::GetWindowWidth(), stats->numEncoders * itemHeightWithSpacing)))
						{
							ImGuiListClipper clipper;
							clipper.Begin(stats->numEncoders, itemHeight);

							while (clipper.Step())
							{
								for (int32_t pos = clipper.DisplayStart; pos < clipper.DisplayEnd; ++pos)
								{
									const bgfx::EncoderStats& encoderStats = stats->encoderStats[pos];

									ImGui::Text("%3d", pos);
									ImGui::SameLine(64.0f);

									const float maxWidth = 30.0f * scale;
									const float cpuMs = float((encoderStats.cpuTimeEnd - encoderStats.cpuTimeBegin) * toCpuMs);
									const float cpuWidth = bx::clamp(cpuMs * scale, 1.0f, maxWidth);

									if (bar(cpuWidth, maxWidth, itemHeight, cpuColor))
									{
										ImGui::SetTooltip("Encoder %d, CPU: %f [ms]"
											, pos
											, cpuMs
										);
									}
								}
							}

							ImGui::EndListBox();
						}

						ImGui::Separator();

						if (ImGui::BeginListBox("Views", ImVec2(ImGui::GetWindowWidth(), stats->numViews * itemHeightWithSpacing)))
						{
							ImGuiListClipper clipper;
							clipper.Begin(stats->numViews, itemHeight);

							while (clipper.Step())
							{
								for (int32_t pos = clipper.DisplayStart; pos < clipper.DisplayEnd; ++pos)
								{
									const bgfx::ViewStats& viewStats = stats->viewStats[pos];

									ImGui::Text("%3d %3d %s", pos, viewStats.view, viewStats.name);

									const float maxWidth = 30.0f * scale;
									const float cpuTimeElapsed = float((viewStats.cpuTimeEnd - viewStats.cpuTimeBegin) * toCpuMs);
									const float gpuTimeElapsed = float((viewStats.gpuTimeEnd - viewStats.gpuTimeBegin) * toGpuMs);
									const float cpuWidth = bx::clamp(cpuTimeElapsed * scale, 1.0f, maxWidth);
									const float gpuWidth = bx::clamp(gpuTimeElapsed * scale, 1.0f, maxWidth);

									ImGui::SameLine(64.0f);

									if (bar(cpuWidth, maxWidth, itemHeight, cpuColor))
									{
										ImGui::SetTooltip("View %d \"%s\", CPU: %f [ms]"
											, pos
											, viewStats.name
											, cpuTimeElapsed
										);
									}

									ImGui::SameLine();
									if (bar(gpuWidth, maxWidth, itemHeight, gpuColor))
									{
										ImGui::SetTooltip("View: %d \"%s\", GPU: %f [ms]"
											, pos
											, viewStats.name
											, gpuTimeElapsed
										);
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
	_input.drawImgui();
	ImGui::End();
}

namespace ImGui
{
	void PushFont(Font::Enum _font)
	{
		PushFont(s_ctx.m_font[_font]);
	}

	void PushEnabled(bool _enabled)
	{
		extern void PushItemFlag(int option, bool enabled);
		PushItemFlag(ImGuiItemFlags_Disabled, !_enabled);
		PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * (_enabled ? 1.0f : 0.5f));
	}

	void PopEnabled()
	{
		extern void PopItemFlag();
		PopItemFlag();
		PopStyleVar();
	}

} // namespace ImGui

#if USE_LOCAL_STB
BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4505); // error C4505: '' : unreferenced local function has been removed
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wunused-function"); // warning: 'int rect_width_compare(const void*, const void*)' defined but not used
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG("-Wunknown-pragmas")
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wtype-limits"); // warning: comparison is always true due to limited range of data type

#	define STBTT_ifloor(_a)   int32_t(bx::floor(_a) )
#	define STBTT_iceil(_a)    int32_t(bx::ceil(_a) )
#	define STBTT_sqrt(_a)     bx::sqrt(_a)
#	define STBTT_pow(_a, _b)  bx::pow(_a, _b)
#	define STBTT_fmod(_a, _b) bx::mod(_a, _b)
#	define STBTT_cos(_a)      bx::cos(_a)
#	define STBTT_acos(_a)     bx::acos(_a)
#	define STBTT_fabs(_a)     bx::abs(_a)
#	define STBTT_strlen(_str) bx::strLen(_str)

#	define STBTT_memcpy(_dst, _src, _numBytes) bx::memCopy(_dst, _src, _numBytes)
#	define STBTT_memset(_dst, _ch, _numBytes)  bx::memSet(_dst, _ch, _numBytes)
#	define STBTT_malloc(_size, _userData)      memAlloc(_size, _userData)
#	define STBTT_free(_ptr, _userData)         memFree(_ptr, _userData)

#	define STB_RECT_PACK_IMPLEMENTATION
#	include <stb/stb_rect_pack.h>
#	define STB_TRUETYPE_IMPLEMENTATION
#	include <stb/stb_truetype.h>
#endif // USE_LOCAL_STB
