#pragma once

#include <glm/glm.hpp>

struct Capsule {
    float radius = 0.3f;
    float halfHeight = 0.5f; // half of the cylinder part (total height = 2*halfHeight + 2*radius)

    // Returns the two endpoints of the capsule's internal line segment
    // given the capsule's center position
    glm::vec3 top(const glm::vec3& center) const {
        return center + glm::vec3(0, halfHeight, 0);
    }

    glm::vec3 bottom(const glm::vec3& center) const {
        return center - glm::vec3(0, halfHeight, 0);
    }

    float totalHeight() const { return 2.0f * halfHeight + 2.0f * radius; }

    // Bottom of the capsule sphere (feet position)
    float feetY(const glm::vec3& center) const {
        return center.y - halfHeight - radius;
    }
};
