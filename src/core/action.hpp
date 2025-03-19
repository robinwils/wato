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

    CameraAction Action;
    double       Delta;
};

struct BuildTower {
};

struct TowerPlacementMode {
    bool Enable;
};

struct TowerCreated {
};
