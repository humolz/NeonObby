#pragma once

#include "ecs/System.h"
#include "ecs/World.h"
#include "core/Input.h"
#include "player/ThirdPersonCamera.h"
#include "components/TransformComponent.h"
#include "components/RigidBodyComponent.h"
#include "components/ColliderComponent.h"
#include "components/PlayerComponent.h"
#include <glm/glm.hpp>

class PlayerControllerSystem : public System {
public:
    PlayerControllerSystem(ThirdPersonCamera& camera) : m_camera(camera) {}

    void update(World& world, float dt) override {
        world.each<TransformComponent, RigidBodyComponent, ColliderComponent, PlayerComponent>(
            [&](Entity, TransformComponent& tc, RigidBodyComponent& rb,
                ColliderComponent& col, PlayerComponent& pc) {
                updatePlayer(tc, rb, col, pc, dt);
            }
        );
    }

    // Set externally by CheckpointSystem
    void setRespawnPosition(const glm::vec3& pos) { m_respawnPos = pos; }

    bool playerDiedThisFrame() const { return m_diedThisFrame; }

private:
    ThirdPersonCamera& m_camera;
    glm::vec3 m_respawnPos{0.0f, 3.0f, 0.0f};
    bool m_diedThisFrame = false;

    void updatePlayer(TransformComponent& tc, RigidBodyComponent& rb,
                      ColliderComponent& col, PlayerComponent& pc, float dt) {
        m_diedThisFrame = false;

        // Skip all input when dead
        if (pc.state == PlayerState::Dead) return;

        glm::vec2 moveInput = Input::moveInput();

        // Coyote time tracking
        if (rb.isGrounded) {
            pc.coyoteTimer = 0.0f;
        } else {
            pc.coyoteTimer += dt;
        }

        // Jump buffer
        if (Input::keyDown(GLFW_KEY_SPACE) && !pc.jumpPressed) {
            pc.jumpBufferTimer = pc.jumpBufferTime;
            pc.jumpPressed = true;
        }
        if (!Input::keyDown(GLFW_KEY_SPACE)) {
            pc.jumpPressed = false;
        }
        pc.jumpBufferTimer -= dt;

        // Crouch / Prone input
        pc.crouchHeld = Input::keyDown(GLFW_KEY_LEFT_CONTROL) || Input::keyDown(GLFW_KEY_C);
        pc.proneHeld = Input::keyDown(GLFW_KEY_Z);

        // State transitions for crouch/prone
        updateStance(tc, rb, col, pc, dt, moveInput);

        // Movement relative to camera
        glm::vec3 forward = m_camera.forward();
        glm::vec3 right = m_camera.right();
        glm::vec3 moveDir = forward * moveInput.y + right * moveInput.x;
        bool hasInput = glm::length(moveInput) > 0.01f;

        // Movement speed depends on stance
        float currentSpeed = pc.moveSpeed;
        if (pc.state == PlayerState::Crouching) {
            currentSpeed = pc.crouchSpeed;
        } else if (pc.state == PlayerState::Prone) {
            currentSpeed = pc.proneSpeed;
        } else if (pc.state == PlayerState::Sliding) {
            // Slide decays
            float horizSpeed = glm::length(glm::vec2(rb.velocity.x, rb.velocity.z));
            if (horizSpeed < pc.slideMinSpeed || !rb.isGrounded) {
                // End slide, go to crouch
                setCapsuleHeight(col, tc, pc.crouchHeight, pc);
                pc.state = PlayerState::Crouching;
            }
            // Slide doesn't allow steering, just decay
            rb.velocity.x *= pc.slideDecay;
            rb.velocity.z *= pc.slideDecay;
            goto afterMovement;
        }

        {
            float controlFactor = rb.isGrounded ? 1.0f : pc.airControl;

            if (hasInput) {
                moveDir = glm::normalize(moveDir);
                rb.velocity.x = moveDir.x * currentSpeed * controlFactor +
                                rb.velocity.x * (1.0f - controlFactor);
                rb.velocity.z = moveDir.z * currentSpeed * controlFactor +
                                rb.velocity.z * (1.0f - controlFactor);

                // Rotate player to face movement direction
                tc.rotation.y = glm::degrees(std::atan2(-moveDir.x, -moveDir.z));
            }
        }

        afterMovement:

        // Jump (not while crouching/prone/sliding)
        if (pc.state != PlayerState::Crouching && pc.state != PlayerState::Prone &&
            pc.state != PlayerState::Sliding) {
            bool canJump = rb.isGrounded || pc.coyoteTimer < pc.coyoteTime;
            if (pc.jumpBufferTimer > 0.0f && canJump) {
                rb.velocity.y = pc.jumpForce;
                rb.isGrounded = false;
                pc.coyoteTimer = pc.coyoteTime;
                pc.jumpBufferTimer = 0.0f;
            }
        }

        // Update state (for non-crouch/prone/slide states)
        if (pc.state != PlayerState::Crouching && pc.state != PlayerState::Prone &&
            pc.state != PlayerState::Sliding) {
            if (rb.isGrounded) {
                pc.state = hasInput ? PlayerState::Running : PlayerState::Idle;
            } else {
                pc.state = rb.velocity.y > 0.0f ? PlayerState::Jumping : PlayerState::Falling;
            }
        }

        // Death plane — flag death for GameScene to handle with animation
        if (tc.position.y < -20.0f) {
            pc.deathCount++;
            m_diedThisFrame = true;
        }
    }

    void updateStance(TransformComponent& tc, RigidBodyComponent& rb,
                      ColliderComponent& col, PlayerComponent& pc,
                      float dt, const glm::vec2& moveInput) {
        if (!rb.isGrounded) return; // Can only change stance on ground

        bool isCrouched = (pc.state == PlayerState::Crouching || pc.state == PlayerState::Sliding);
        bool isProne = (pc.state == PlayerState::Prone);

        // Enter prone (Z key, highest priority)
        if (pc.proneHeld && !isProne) {
            setCapsuleHeight(col, tc, pc.proneHeight, pc);
            pc.state = PlayerState::Prone;
            return;
        }

        // Exit prone
        if (isProne && !pc.proneHeld) {
            // Try to stand up to crouch first
            setCapsuleHeight(col, tc, pc.crouchHeight, pc);
            pc.state = PlayerState::Crouching;
            return;
        }

        // Enter crouch / slide
        if (pc.crouchHeld && !isCrouched && !isProne) {
            float horizSpeed = glm::length(glm::vec2(rb.velocity.x, rb.velocity.z));
            if (horizSpeed > 6.0f) {
                // Initiate slide
                setCapsuleHeight(col, tc, pc.crouchHeight, pc);
                pc.state = PlayerState::Sliding;
            } else {
                setCapsuleHeight(col, tc, pc.crouchHeight, pc);
                pc.state = PlayerState::Crouching;
            }
            return;
        }

        // Exit crouch
        if (isCrouched && !pc.crouchHeld) {
            // TODO: clearance check (raycast upward) before standing
            setCapsuleHeight(col, tc, pc.standHeight, pc);
            // State will be updated by main state logic
            pc.state = PlayerState::Idle;
            return;
        }
    }

    void setCapsuleHeight(ColliderComponent& col, TransformComponent& tc,
                          float newHalfHeight, PlayerComponent& pc) {
        float oldFeetY = tc.position.y - col.capsule.halfHeight - col.capsule.radius;
        col.capsule.halfHeight = newHalfHeight;
        // Keep feet on the ground
        tc.position.y = oldFeetY + col.capsule.halfHeight + col.capsule.radius;
        // Scale visual to match
        tc.scale.y = (col.capsule.halfHeight * 2.0f + col.capsule.radius * 2.0f) /
                     (pc.standHeight * 2.0f + col.capsule.radius * 2.0f);
    }
};
