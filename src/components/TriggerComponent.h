#pragma once

#include <glm/glm.hpp>
#include <cstdint>

enum class TriggerType : uint8_t {
    KillZone,
    JumpPad,
    Checkpoint,
    LevelFinish,
    SpeedBoost
};

struct TriggerComponent {
    TriggerType type = TriggerType::KillZone;
    float strength = 0.0f;  // jump pad force, speed multiplier
    int checkpointIndex = 0;
    bool playerInside = false;
};
