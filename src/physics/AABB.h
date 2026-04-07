#pragma once

#include <glm/glm.hpp>
#include <algorithm>

struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};

    glm::vec3 center() const { return (min + max) * 0.5f; }
    glm::vec3 halfExtents() const { return (max - min) * 0.5f; }

    static AABB fromCenterSize(const glm::vec3& center, const glm::vec3& size) {
        glm::vec3 half = size * 0.5f;
        return {center - half, center + half};
    }

    bool overlaps(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }

    glm::vec3 closestPoint(const glm::vec3& p) const {
        return glm::clamp(p, min, max);
    }
};
