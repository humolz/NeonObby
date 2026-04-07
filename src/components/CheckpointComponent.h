#pragma once

#include <glm/glm.hpp>

struct CheckpointComponent {
    int index = 0;
    glm::vec3 respawnPosition{0.0f, 3.0f, 0.0f};
    bool activated = false;
};
