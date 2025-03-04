#pragma once

#include <glm/glm.hpp>

struct CameraMovement {
    enum CameraAction {
        CameraLeft,
        CameraRight,
        CameraForward,
        CameraBack,
        CameraZoom,
    };

    CameraAction action;
    double       delta;
};

struct BuildTower {
};

struct TowerPlacementMode {
    bool enable;
};
