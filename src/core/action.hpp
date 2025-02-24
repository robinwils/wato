#pragma once

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

struct CameraMovement {
    Action action;
    double time_delta;
};
