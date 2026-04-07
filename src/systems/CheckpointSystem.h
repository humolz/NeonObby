#pragma once

#include "ecs/World.h"
#include "components/TransformComponent.h"
#include "components/RigidBodyComponent.h"
#include "components/PlayerComponent.h"
#include "components/CheckpointComponent.h"
#include <glm/glm.hpp>

class CheckpointSystem {
public:
    void onCheckpointReached(World& world, int index) {
        if (index <= m_lastCheckpoint) return;
        m_lastCheckpoint = index;

        // Find this checkpoint's respawn position
        world.each<CheckpointComponent>(
            [&](Entity, CheckpointComponent& cp) {
                if (cp.index == index) {
                    m_respawnPosition = cp.respawnPosition;
                }
            }
        );
    }

    void respawnPlayer(World& world) {
        world.each<TransformComponent, RigidBodyComponent, PlayerComponent>(
            [&](Entity, TransformComponent& tc, RigidBodyComponent& rb, PlayerComponent&) {
                tc.position = m_respawnPosition;
                rb.velocity = glm::vec3(0.0f);
            }
        );
    }

    glm::vec3 respawnPosition() const { return m_respawnPosition; }
    int lastCheckpoint() const { return m_lastCheckpoint; }

    void reset() {
        m_lastCheckpoint = -1;
        m_respawnPosition = glm::vec3(0.0f, 3.0f, 0.0f);
    }

private:
    int m_lastCheckpoint = -1;
    glm::vec3 m_respawnPosition{0.0f, 3.0f, 0.0f};
};
