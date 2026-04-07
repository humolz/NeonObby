#pragma once

#include <glm/glm.hpp>
#include "ecs/Entity.h"

struct RigidBodyComponent {
    glm::vec3 velocity{0.0f};
    glm::vec3 acceleration{0.0f};
    float mass = 1.0f;
    float gravityScale = 1.0f;
    bool isKinematic = false;  // true = position set directly (platforms)
    bool isGrounded = false;
    float friction = 0.92f;
    Entity groundEntity = Entity::null();
};
