#pragma once

#include <concepts>

class WatoWindow;

template <typename T>
concept RendererConcept = requires(T r, WatoWindow& win) {
    r.Init(win);
    r.Resize(win);
    r.Clear();
    r.Render();
    { r.IsInitialized() } -> std::convertible_to<bool>;
};
