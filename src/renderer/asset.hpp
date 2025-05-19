#pragma once

#include <string>

static constexpr const char* kSearchPaths[] = {"assets/models", "assets/textures"};

std::string FindAsset(const char* aName);
