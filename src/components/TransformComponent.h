#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f}; // Euler angles in degrees
    glm::vec3 scale{1.0f};

    glm::mat4 matrix() const {
        glm::mat4 m(1.0f);
        m = glm::translate(m, position);
        m = glm::rotate(m, glm::radians(rotation.x), {1, 0, 0});
        m = glm::rotate(m, glm::radians(rotation.y), {0, 1, 0});
        m = glm::rotate(m, glm::radians(rotation.z), {0, 0, 1});
        m = glm::scale(m, scale);
        return m;
    }
};
