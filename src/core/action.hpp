#pragma once

enum Action {
    CameraLeft,
    CameraRight,
    CameraForward,
    CameraBack,
    BuildTower,
};

struct CameraMovement {
    Action action;
    double time_delta;
};
