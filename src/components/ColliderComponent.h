#pragma once

#include "physics/AABB.h"
#include "physics/Capsule.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

enum class ColliderType { AABBCollider, CapsuleCollider };

struct ColliderComponent {
    ColliderType type = ColliderType::AABBCollider;
    // For AABB: half-extents (actual AABB computed from position + halfExtents)
    glm::vec3 halfExtents{0.5f};
    // For Capsule:
    Capsule capsule;

    AABB getAABB(const glm::vec3& position, const glm::vec3& scale) const {
        glm::vec3 he = halfExtents * scale;
        return {position - he, position + he};
    }

    // World AABB that accounts for rotation (bounding box of rotated OBB)
    AABB getWorldAABB(const glm::vec3& position, const glm::vec3& scale,
                      const glm::vec3& rotation) const {
        glm::vec3 he = halfExtents * scale;

        // Build 3x3 rotation matrix
        glm::mat4 rot(1.0f);
        rot = glm::rotate(rot, glm::radians(rotation.x), {1, 0, 0});
        rot = glm::rotate(rot, glm::radians(rotation.y), {0, 1, 0});
        rot = glm::rotate(rot, glm::radians(rotation.z), {0, 0, 1});

        // Compute AABB of the rotated box using abs-matrix method
        glm::vec3 newHe(0.0f);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                newHe[i] += std::abs(rot[j][i]) * he[j];
            }
        }

        return {position - newHe, position + newHe};
    }
};
