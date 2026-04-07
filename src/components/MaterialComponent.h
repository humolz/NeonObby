#pragma once

#include <glm/glm.hpp>

struct MaterialComponent {
    glm::vec3 baseColor{0.5f, 0.5f, 0.5f};
    glm::vec3 emissionColor{0.0f, 0.0f, 0.0f};
    float emissionStrength = 0.0f;
};
