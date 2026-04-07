#pragma once

#include "renderer/Model.h"

struct ModelComponent {
    Model* model = nullptr;
    int currentClip = -1;
    int previousClip = -1;
    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool looping = true;            // current clip's looping behavior
    // Optional model-space yaw offset (degrees) — useful if the model faces -Z by default
    float yawOffset = 0.0f;
};
