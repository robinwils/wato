#pragma once

#include <glm/glm.hpp>

struct CameraMovement {
    enum CameraAction {
        CameraLeft,
        CameraRight,
        CameraForward,
        CameraBack,
    };

    CameraAction action;
    double       time_delta;
};

struct BuildTower {
};

struct TowerPlacementMode {
    bool enable;
};
