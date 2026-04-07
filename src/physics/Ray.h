#pragma once

#include <glm/glm.hpp>

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction; // should be normalized

    glm::vec3 pointAt(float t) const { return origin + direction * t; }
};

struct RayHit {
    bool hit = false;
    float distance = 0.0f;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
};
