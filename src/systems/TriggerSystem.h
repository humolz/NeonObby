#pragma once

#include "ecs/World.h"
#include "components/TransformComponent.h"
#include "components/RigidBodyComponent.h"
#include "components/ColliderComponent.h"
#include "components/PlayerComponent.h"
#include "components/TriggerComponent.h"
#include "components/CheckpointComponent.h"
#include "physics/AABB.h"
#include "physics/Capsule.h"
#include <glm/glm.hpp>

struct TriggerEvents {
    bool playerDied = false;
    bool levelFinished = false;
    bool speedBoosted = false;
    bool jumpPadHit = false;
    int checkpointReached = -1;
};

class TriggerSystem {
public:
    TriggerEvents update(World& world, float dt) {
        TriggerEvents events;

        // Find the player entity
        Entity playerEntity = Entity::null();
        TransformComponent* playerTc = nullptr;
        RigidBodyComponent* playerRb = nullptr;
        PlayerComponent* playerPc = nullptr;
        ColliderComponent* playerCol = nullptr;

        world.each<TransformComponent, RigidBodyComponent, PlayerComponent, ColliderComponent>(
            [&](Entity e, TransformComponent& tc, RigidBodyComponent& rb,
                PlayerComponent& pc, ColliderComponent& col) {
                playerEntity = e;
                playerTc = &tc;
                playerRb = &rb;
                playerPc = &pc;
                playerCol = &col;
            }
        );

        if (!playerEntity.valid()) return events;

        // Build player AABB from capsule
        AABB playerBox;
        if (playerCol->type == ColliderType::CapsuleCollider) {
            glm::vec3 top = playerCol->capsule.top(playerTc->position);
            glm::vec3 bot = playerCol->capsule.bottom(playerTc->position);
            float r = playerCol->capsule.radius;
            playerBox.min = glm::min(top, bot) - glm::vec3(r);
            playerBox.max = glm::max(top, bot) + glm::vec3(r);
        } else {
            playerBox = playerCol->getAABB(playerTc->position, playerTc->scale);
        }

        // Check all triggers (use rotation-aware AABB for rotating objects)
        world.each<TransformComponent, ColliderComponent, TriggerComponent>(
            [&](Entity e, TransformComponent& tc, ColliderComponent& col, TriggerComponent& trigger) {
                // Use rotation-aware AABB for spinners/rotating triggers
                AABB triggerBox = (tc.rotation.x != 0.0f || tc.rotation.y != 0.0f || tc.rotation.z != 0.0f)
                    ? col.getWorldAABB(tc.position, tc.scale, tc.rotation)
                    : col.getAABB(tc.position, tc.scale);
                bool overlapping = playerBox.overlaps(triggerBox);

                if (overlapping && !trigger.playerInside) {
                    trigger.playerInside = true;
                    onTriggerEnter(trigger, *playerTc, *playerRb, *playerPc, events, world);
                } else if (overlapping && trigger.playerInside && trigger.type == TriggerType::KillZone) {
                    // KillZones keep firing while inside (for spinning obstacles)
                    onTriggerEnter(trigger, *playerTc, *playerRb, *playerPc, events, world);
                } else if (!overlapping && trigger.playerInside) {
                    trigger.playerInside = false;
                }
            }
        );

        return events;
    }

private:
    void onTriggerEnter(TriggerComponent& trigger,
                        TransformComponent& playerTc, RigidBodyComponent& playerRb,
                        PlayerComponent& playerPc, TriggerEvents& events, World& world) {
        switch (trigger.type) {
        case TriggerType::KillZone:
            events.playerDied = true;
            playerPc.deathCount++;
            break;

        case TriggerType::JumpPad:
            playerRb.velocity.y = trigger.strength;
            playerRb.isGrounded = false;
            events.jumpPadHit = true;
            break;

        case TriggerType::SpeedBoost: {
            glm::vec3 horizVel(playerRb.velocity.x, 0, playerRb.velocity.z);
            float len = glm::length(horizVel);
            if (len > 0.1f) {
                glm::vec3 dir = horizVel / len;
                playerRb.velocity.x = dir.x * trigger.strength;
                playerRb.velocity.z = dir.z * trigger.strength;
            }
            events.speedBoosted = true;
            break;
        }

        case TriggerType::Checkpoint:
            events.checkpointReached = trigger.checkpointIndex;
            // Activate the checkpoint
            world.each<CheckpointComponent>(
                [&](Entity, CheckpointComponent& cp) {
                    if (cp.index == trigger.checkpointIndex && !cp.activated) {
                        cp.activated = true;
                    }
                }
            );
            break;

        case TriggerType::LevelFinish:
            events.levelFinished = true;
            break;
        }
    }
};
