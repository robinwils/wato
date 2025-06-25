#pragma once

#include <bimg/decode.h>

#include <entt/core/hashed_string.hpp>
#include <entt/resource/cache.hpp>

#include "resource/model_loader.hpp"
#include "resource/shader_loader.hpp"
#include "resource/texture_loader.hpp"

using namespace entt::literals;

using TextureCache = entt::resource_cache<bgfx::TextureHandle, TextureLoader>;
using ShaderCache  = entt::resource_cache<Shader, ShaderLoader>;
using ModelCache   = entt::resource_cache<Model, ModelLoader>;
