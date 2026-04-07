#pragma once

#include <cstdint>

enum class PlayerState : uint8_t {
    Idle,
    Running,
    Jumping,
    Falling,
    Crouching,
    Prone,
    Sliding,
    Dead
};

struct PlayerComponent {
    PlayerState state = PlayerState::Idle;
    float moveSpeed = 8.0f;
    float crouchSpeed = 4.0f;
    float proneSpeed = 1.5f;
    float jumpForce = 10.0f;
    float airControl = 0.4f;
    float coyoteTime = 0.1f;
    float coyoteTimer = 0.0f;    // time since last grounded
    float jumpBufferTime = 0.1f;
    float jumpBufferTimer = 0.0f; // time since jump pressed
    bool jumpPressed = false;
    int deathCount = 0;

    // Crouch / Prone
    float standHeight = 1.0f;     // capsule half-height standing
    float crouchHeight = 0.6f;    // capsule half-height crouching
    float proneHeight = 0.3f;     // capsule half-height prone
    float slideSpeed = 12.0f;     // initial slide burst
    float slideDecay = 0.92f;     // slide speed decay per frame
    float slideMinSpeed = 3.0f;   // speed at which slide ends
    bool crouchHeld = false;
    bool proneHeld = false;
};
