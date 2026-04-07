#pragma once

#include "scene/Scene.h"
#include "systems/RenderSystem.h"
#include "systems/PlayerControllerSystem.h"
#include "systems/ObstacleSystem.h"
#include "systems/TriggerSystem.h"
#include "systems/CheckpointSystem.h"
#include "systems/TimerSystem.h"
#include "level/LevelLoader.h"
#include "renderer/Shader.h"
#include "renderer/Camera.h"
#include "renderer/MeshCache.h"
#include "renderer/PostProcess.h"
#include "renderer/ScreenQuad.h"
#include "renderer/ParticleSystem.h"
#include "renderer/DebugRenderer.h"
#include "renderer/Model.h"
#include "renderer/ModelLoader.h"
#include "player/ThirdPersonCamera.h"
#include "physics/PhysicsWorld.h"
#include "audio/AudioEngine.h"
#include "components/TransformComponent.h"
#include "components/MeshComponent.h"
#include "components/MaterialComponent.h"
#include "components/RigidBodyComponent.h"
#include "components/ColliderComponent.h"
#include "components/PlayerComponent.h"
#include "components/ObstacleComponent.h"
#include "components/CheckpointComponent.h"
#include "components/TriggerComponent.h"
#include "components/ModelComponent.h"
#include "ui/HUD.h"
#include "ui/PauseMenu.h"
#include "ui/LevelCompleteUI.h"
#include "ui/DeathOverlay.h"
#include "core/Input.h"
#include "core/Paths.h"

#include <string>
#include <functional>
#include <iostream>

class GameScene : public Scene {
public:
    using MenuCallback = std::function<void()>;
    using NextLevelCallback = std::function<void(int)>;
    using CompleteCallback = std::function<void(int levelIdx, float time, int deaths)>;

    GameScene(Shader& neonShader, Shader& skyboxShader, Shader& particleShader,
              Shader& debugShader, Shader& skinnedShader, MeshCache& meshes,
              AudioEngine& audio, Model* playerModel)
        : m_neonShader(neonShader), m_skyboxShader(skyboxShader),
          m_particleShader(particleShader), m_debugShader(debugShader),
          m_skinnedShader(skinnedShader), m_meshes(meshes), m_audio(audio),
          m_playerModel(playerModel) {}

    ThirdPersonCamera& camera() { return m_camera; }

    void setLevelPath(const std::string& path) { m_levelPath = path; }
    void setLevelIndex(int idx) { m_levelIndex = idx; }
    void setTotalLevels(int n) { m_totalLevels = n; }
    void setOnMainMenu(MenuCallback cb) { m_onMainMenu = std::move(cb); }
    void setOnNextLevel(NextLevelCallback cb) { m_onNextLevel = std::move(cb); }
    void setOnComplete(CompleteCallback cb) { m_onComplete = std::move(cb); }

    bool isPaused() const { return m_paused; }
    bool isLevelComplete() const { return m_levelComplete; }
    bool wantsCursor() const { return m_paused || m_levelComplete; }
    int levelIndex() const { return m_levelIndex; }
    bool debugEnabled() const { return m_debugRenderer.enabled; }

    void onEnter() override {
        m_playerController = std::make_unique<PlayerControllerSystem>(m_camera);
        m_particles.init(2048);
        m_debugRenderer.init();
        loadLevel();
        spawnPlayer();
        countCheckpoints();
        setupParticleEmitters();
        m_levelTimer.start();
    }

    void update(float dt) override {
        // Pause toggle with Escape
        if (Input::keyDown(GLFW_KEY_ESCAPE) && !m_escWasDown) {
            if (m_levelComplete) {
                // Esc during level complete does nothing
            } else {
                m_paused = !m_paused;
            }
        }
        m_escWasDown = Input::keyDown(GLFW_KEY_ESCAPE);

        // Debug toggle with F3
        if (Input::keyDown(GLFW_KEY_F3) && !m_f3WasDown) {
            m_debugRenderer.enabled = !m_debugRenderer.enabled;
        }
        m_f3WasDown = Input::keyDown(GLFW_KEY_F3);

        if (m_paused || m_levelComplete) return;

        // Death animation in progress — just tick the timer
        if (m_deathAnimating) {
            m_deathTimer += dt;
            m_particles.update(dt);
            if (m_deathTimer > 2.2f) {
                // Animation done — respawn
                m_deathAnimating = false;
                m_deathTimer = 0.0f;
                m_checkpointSystem.respawnPlayer(m_world);
                // Restore player to normal state
                if (m_world.alive(m_player)) {
                    auto& pc = m_world.get<PlayerComponent>(m_player);
                    pc.state = PlayerState::Idle;
                    auto& rb = m_world.get<RigidBodyComponent>(m_player);
                    rb.isKinematic = false;
                }
            }
            return;
        }

        m_time += dt;

        // Speed boost FOV effect
        if (m_speedBoostTimer > 0.0f) {
            m_speedBoostTimer -= dt;
            m_camera.targetFov = 75.0f; // zoomed out during boost
        } else {
            m_camera.targetFov = m_camera.baseFov;
        }

        // Update obstacles (moving platforms, spinners, timed)
        m_obstacleSystem.update(m_world, dt);

        // Player controller
        m_playerController->update(m_world, dt);

        // Moving platform carry — must run BEFORE physics so the player rides
        // with the platform. If applied AFTER physics, the collision response
        // already pushed the player up by the same delta and we'd double-count
        // it, causing the capsule to bounce off the platform every frame
        // (which retriggered the land SFX repeatedly).
        applyMovingPlatformCarry();

        // Physics
        m_physics.step(m_world, dt);

        // Triggers
        TriggerEvents events = m_triggerSystem.update(m_world, dt);

        if (events.checkpointReached >= 0) {
            m_checkpointSystem.onCheckpointReached(m_world, events.checkpointReached);
            m_playerController->setRespawnPosition(m_checkpointSystem.respawnPosition());

            // Checkpoint sparkle particles
            glm::vec3 cpPos = m_checkpointSystem.respawnPosition();
            BurstConfig burst;
            burst.position = cpPos;
            burst.count = 40;
            burst.velocityMin = {-2.0f, 1.0f, -2.0f};
            burst.velocityMax = {2.0f, 5.0f, 2.0f};
            burst.colorStart = {0.0f, 0.8f, 1.0f, 1.0f};
            burst.colorEnd = {0.0f, 0.3f, 1.0f, 0.0f};
            burst.sizeStart = 0.2f;
            burst.sizeEnd = 0.02f;
            burst.lifeMin = 0.8f;
            burst.lifeMax = 1.8f;
            burst.gravity = -1.0f;
            m_particles.spawnBurst(burst);

            // Audio
            m_audio.play3D(SFX::Checkpoint, cpPos);
        }

        // Speed boost event tracking
        if (events.speedBoosted) {
            m_speedBoostTimer = 1.0f; // 1 second of FOV effect
        }

        if (events.jumpPadHit) {
            if (m_world.alive(m_player)) {
                auto& tc = m_world.get<TransformComponent>(m_player);
                m_audio.play3D(SFX::Jump, tc.position);
            }
        }

        if (events.playerDied || m_playerController->playerDiedThisFrame()) {
            startDeathAnimation();
        }

        if (events.levelFinished && !m_levelComplete) {
            m_levelComplete = true;
            m_levelTimer.stop();
            m_audio.play(SFX::LevelComplete);
            // Notify save system
            if (m_onComplete) {
                int deaths = 0;
                if (m_world.alive(m_player)) {
                    deaths = m_world.get<PlayerComponent>(m_player).deathCount;
                }
                m_onComplete(m_levelIndex, m_levelTimer.elapsed(), deaths);
            }
        }

        // Detect landing for SFX
        if (m_world.alive(m_player)) {
            auto& rb = m_world.get<RigidBodyComponent>(m_player);
            if (rb.isGrounded && !m_wasGrounded) {
                auto& tc = m_world.get<TransformComponent>(m_player);
                m_audio.play3D(SFX::Land, tc.position);
            }
            m_wasGrounded = rb.isGrounded;
        }

        // Detect jump for SFX
        if (m_world.alive(m_player)) {
            auto& pc = m_world.get<PlayerComponent>(m_player);
            if (pc.state == PlayerState::Jumping && m_prevPlayerState != PlayerState::Jumping) {
                auto& tc = m_world.get<TransformComponent>(m_player);
                m_audio.play3D(SFX::Jump, tc.position);
            }
            m_prevPlayerState = pc.state;
        }

        // Level timer
        m_levelTimer.update(dt);

        // Update camera to follow player
        if (m_world.alive(m_player)) {
            auto& tc = m_world.get<TransformComponent>(m_player);
            m_camera.update(tc.position, dt);
            m_audio.setListenerPos(m_camera.position());

            // Rotate player to face the camera's forward direction (away from camera)
            tc.rotation.y = m_camera.yaw;
        }

        // Update player animation based on player state
        updatePlayerAnimation(dt);

        // Update particles
        m_particles.update(dt);

        // Restart level with R
        if (Input::keyDown(GLFW_KEY_R) && !m_rWasDown) {
            restartLevel();
        }
        m_rWasDown = Input::keyDown(GLFW_KEY_R);
    }

    void render(float /*alpha*/) override {
        // Skybox
        m_skyboxShader.bind();
        glDepthMask(GL_FALSE);
        m_skyQuad.draw();
        glDepthMask(GL_TRUE);

        // Render all entities
        glm::mat4 viewProj = m_camera.projectionMatrix(m_aspect) * m_camera.viewMatrix();
        m_neonShader.bind();
        m_neonShader.setMat4("u_viewProj", viewProj);
        m_neonShader.setVec3("u_lightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f)));
        m_neonShader.setVec3("u_viewPos", m_camera.position());
        m_neonShader.setFloat("u_time", m_time);

        m_world.each<TransformComponent, MeshComponent, MaterialComponent>(
            [&](Entity e, TransformComponent& t, MeshComponent& mc, MaterialComponent& mat) {
                if (t.scale.x < 0.01f && t.scale.y < 0.01f && t.scale.z < 0.01f) return;

                // Pulsing glow for checkpoints and triggers
                float emStrength = mat.emissionStrength;
                if (m_world.has<CheckpointComponent>(e)) {
                    auto& cp = m_world.get<CheckpointComponent>(e);
                    if (cp.activated) {
                        emStrength = mat.emissionStrength * (0.6f + 0.4f * std::sin(m_time * 4.0f));
                    }
                } else if (m_world.has<TriggerComponent>(e)) {
                    auto& trig = m_world.get<TriggerComponent>(e);
                    if (trig.type == TriggerType::JumpPad || trig.type == TriggerType::SpeedBoost) {
                        emStrength = mat.emissionStrength * (0.7f + 0.3f * std::sin(m_time * 3.0f));
                    }
                }

                m_neonShader.setMat4("u_model", t.matrix());
                m_neonShader.setVec3("u_baseColor", mat.baseColor);
                m_neonShader.setVec3("u_emissionColor", mat.emissionColor);
                m_neonShader.setFloat("u_emissionStrength", emStrength);
                m_meshes.get(mc.type).draw();
            }
        );

        // Skinned models (player character)
        m_skinnedShader.bind();
        m_skinnedShader.setMat4("u_viewProj", viewProj);
        m_skinnedShader.setVec3("u_lightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f)));
        m_skinnedShader.setVec3("u_viewPos", m_camera.position());
        m_skinnedShader.setFloat("u_time", m_time);

        m_world.each<TransformComponent, ModelComponent, MaterialComponent>(
            [&](Entity, TransformComponent& t, ModelComponent& mc, MaterialComponent& mat) {
                if (!mc.model) return;

                mc.model->computeBoneMatrices(mc.currentClip, mc.currentTime, mc.looping, m_boneMatrixScratch);

                // Apply yaw offset to model rotation
                glm::mat4 model = glm::translate(glm::mat4(1.0f), t.position)
                                * glm::rotate(glm::mat4(1.0f), glm::radians(t.rotation.y + mc.yawOffset), glm::vec3(0, 1, 0))
                                * glm::rotate(glm::mat4(1.0f), glm::radians(t.rotation.x), glm::vec3(1, 0, 0))
                                * glm::rotate(glm::mat4(1.0f), glm::radians(t.rotation.z), glm::vec3(0, 0, 1))
                                * glm::scale(glm::mat4(1.0f), t.scale);

                m_skinnedShader.setMat4("u_model", model);
                m_skinnedShader.setVec3("u_baseColor", mat.baseColor);
                m_skinnedShader.setVec3("u_emissionColor", mat.emissionColor);
                m_skinnedShader.setFloat("u_emissionStrength", mat.emissionStrength);

                int boneCount = std::min(static_cast<int>(m_boneMatrixScratch.size()), 64);
                if (boneCount > 0) {
                    m_skinnedShader.setMat4Array("u_bones", m_boneMatrixScratch.data(), boneCount);
                }

                mc.model->mesh.draw();
            }
        );

        // Particles (inside post-process for bloom)
        m_particleShader.bind();
        m_particleShader.setMat4("u_viewProj", viewProj);
        // Billboard vectors from view matrix
        glm::mat4 view = m_camera.viewMatrix();
        glm::vec3 camRight = {view[0][0], view[1][0], view[2][0]};
        glm::vec3 camUp = {view[0][1], view[1][1], view[2][1]};
        m_particleShader.setVec3("u_cameraRight", camRight);
        m_particleShader.setVec3("u_cameraUp", camUp);
        m_particles.render(viewProj, camRight, camUp);
    }

    // Called AFTER post-process, renders directly to screen
    void renderDebug(float aspect) {
        if (!m_debugRenderer.enabled) return;

        glm::mat4 viewProj = m_camera.projectionMatrix(aspect) * m_camera.viewMatrix();
        m_debugShader.bind();
        m_debugShader.setMat4("u_viewProj", viewProj);

        // Draw all colliders
        m_world.each<TransformComponent, ColliderComponent>(
            [&](Entity, TransformComponent& tc, ColliderComponent& col) {
                glm::vec3 color = {0.0f, 1.0f, 0.0f}; // green for static
                if (col.type == ColliderType::CapsuleCollider) {
                    color = {0.0f, 1.0f, 1.0f}; // cyan for player
                    m_debugRenderer.drawCapsule(tc.position, col.capsule.halfHeight,
                                                 col.capsule.radius, color);
                } else {
                    bool hasRotation = (tc.rotation.x != 0.0f || tc.rotation.y != 0.0f || tc.rotation.z != 0.0f);
                    AABB box = hasRotation
                        ? col.getWorldAABB(tc.position, tc.scale, tc.rotation)
                        : col.getAABB(tc.position, tc.scale);
                    m_debugRenderer.drawAABB(box, color);
                }
            }
        );

        // Draw triggers in different colors
        m_world.each<TransformComponent, ColliderComponent, TriggerComponent>(
            [&](Entity, TransformComponent& tc, ColliderComponent& col, TriggerComponent& trig) {
                glm::vec3 color;
                switch (trig.type) {
                case TriggerType::KillZone:    color = {1.0f, 0.0f, 0.0f}; break;
                case TriggerType::JumpPad:     color = {1.0f, 1.0f, 0.0f}; break;
                case TriggerType::SpeedBoost:  color = {0.0f, 1.0f, 0.5f}; break;
                case TriggerType::Checkpoint:  color = {0.0f, 0.5f, 1.0f}; break;
                case TriggerType::LevelFinish: color = {1.0f, 0.0f, 1.0f}; break;
                }
                AABB box = col.getAABB(tc.position, tc.scale);
                m_debugRenderer.drawAABB(box, color);
            }
        );

        m_debugRenderer.flush(viewProj);
    }

    void renderUI() override {
        // HUD (always visible)
        HUDData hudData;
        hudData.timerElapsed = m_levelTimer.elapsed();
        hudData.timerRunning = m_levelTimer.running();
        hudData.levelComplete = m_levelComplete;
        hudData.checkpointsCurrent = m_checkpointSystem.lastCheckpoint() + 1;
        hudData.checkpointsTotal = m_totalCheckpoints;

        if (m_world.alive(m_player)) {
            auto& pc = m_world.get<PlayerComponent>(m_player);
            auto& rb = m_world.get<RigidBodyComponent>(m_player);
            hudData.deathCount = pc.deathCount;
            hudData.playerSpeed = glm::length(glm::vec2(rb.velocity.x, rb.velocity.z));
        }

        m_hud.render(hudData);

        // Debug info
        if (m_debugRenderer.enabled) {
            ImGui::SetNextWindowPos(ImVec2(10, 120), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.6f);
            ImGui::Begin("##debug", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "DEBUG MODE (F3)");
            ImGui::Text("Particles: %d", m_particles.aliveCount());
            if (m_world.alive(m_player)) {
                auto& tc = m_world.get<TransformComponent>(m_player);
                auto& rb = m_world.get<RigidBodyComponent>(m_player);
                ImGui::Text("Pos: %.1f, %.1f, %.1f", tc.position.x, tc.position.y, tc.position.z);
                ImGui::Text("Vel: %.1f, %.1f, %.1f", rb.velocity.x, rb.velocity.y, rb.velocity.z);
                ImGui::Text("Grounded: %s", rb.isGrounded ? "yes" : "no");
            }
            ImGui::Text("FOV: %.1f", m_camera.fov);
            ImGui::End();
        }

        // Death animation overlay
        if (m_deathAnimating) {
            m_deathOverlay.render(m_deathTimer);
            return; // Don't show pause/complete UI during death
        }

        // Level complete overlay
        if (m_levelComplete) {
            int deaths = 0;
            if (m_world.alive(m_player)) {
                deaths = m_world.get<PlayerComponent>(m_player).deathCount;
            }
            bool hasNext = m_levelIndex < m_totalLevels - 1;
            LevelCompleteAction act = m_completeUI.render(m_levelTimer.elapsed(), deaths, hasNext);

            switch (act) {
            case LevelCompleteAction::NextLevel:
                if (m_onNextLevel) m_onNextLevel(m_levelIndex + 1);
                break;
            case LevelCompleteAction::Restart:
                restartLevel();
                break;
            case LevelCompleteAction::MainMenu:
                if (m_onMainMenu) m_onMainMenu();
                break;
            default:
                break;
            }
            return;
        }

        // Pause menu overlay
        if (m_paused) {
            PauseAction act = m_pauseMenu.render();
            switch (act) {
            case PauseAction::Resume:
                m_paused = false;
                break;
            case PauseAction::Restart:
                m_paused = false;
                restartLevel();
                break;
            case PauseAction::MainMenu:
                if (m_onMainMenu) m_onMainMenu();
                break;
            default:
                break;
            }
        }
    }

    void setAspect(float a) { m_aspect = a; }

private:
    Shader& m_neonShader;
    Shader& m_skyboxShader;
    Shader& m_particleShader;
    Shader& m_debugShader;
    Shader& m_skinnedShader;
    MeshCache& m_meshes;
    AudioEngine& m_audio;
    Model* m_playerModel = nullptr;
    std::vector<glm::mat4> m_boneMatrixScratch;
    ThirdPersonCamera m_camera;
    PhysicsWorld m_physics;
    std::unique_ptr<PlayerControllerSystem> m_playerController;
    ObstacleSystem m_obstacleSystem;
    TriggerSystem m_triggerSystem;
    CheckpointSystem m_checkpointSystem;
    TimerSystem m_levelTimer;
    ParticleSystem m_particles;
    DebugRenderer m_debugRenderer;
    HUD m_hud;
    PauseMenu m_pauseMenu;
    LevelCompleteUI m_completeUI;
    DeathOverlay m_deathOverlay;
    ScreenQuad m_skyQuad;
    Entity m_player;
    float m_time = 0.0f;
    float m_aspect = 16.0f / 9.0f;
    bool m_levelComplete = false;
    bool m_paused = false;
    bool m_escWasDown = false;
    bool m_rWasDown = false;
    bool m_f3WasDown = false;
    bool m_deathAnimating = false;
    float m_deathTimer = 0.0f;
    float m_speedBoostTimer = 0.0f;
    bool m_wasGrounded = false;
    PlayerState m_prevPlayerState = PlayerState::Idle;
    std::string m_levelPath;
    LevelData m_levelData;
    int m_levelIndex = 0;
    int m_totalLevels = 3;
    int m_totalCheckpoints = 0;
    MenuCallback m_onMainMenu;
    NextLevelCallback m_onNextLevel;
    CompleteCallback m_onComplete;

    void spawnPlayer() {
        m_player = m_world.create();
        auto& tc = m_world.add<TransformComponent>(m_player);
        tc.position = m_levelData.spawnPoint;
        // Scale tuned so the model roughly matches the capsule height
        tc.scale = {1.0f, 1.0f, 1.0f};
        m_world.add<MaterialComponent>(m_player, {{0.1f, 0.1f, 0.15f}, {0.0f, 0.9f, 1.0f}, 1.5f});
        auto& rb = m_world.add<RigidBodyComponent>(m_player);
        rb.gravityScale = 1.0f;
        auto& col = m_world.add<ColliderComponent>(m_player);
        col.type = ColliderType::CapsuleCollider;
        col.capsule.radius = 0.3f;
        col.capsule.halfHeight = 0.5f;
        m_world.add<PlayerComponent>(m_player);

        // Add the skinned model if available; fall back to capsule mesh
        if (m_playerModel) {
            ModelComponent mc;
            mc.model = m_playerModel;
            mc.currentClip = m_playerModel->findClip("Walk");
            if (mc.currentClip < 0 && !m_playerModel->clips.empty()) mc.currentClip = 0;
            mc.previousClip = mc.currentClip;
            mc.yawOffset = 0.0f; // glTF models from Blender typically face -Z (forward)
            m_world.add<ModelComponent>(m_player, mc);
        } else {
            m_world.add<MeshComponent>(m_player, {MeshType::Capsule});
        }

        m_playerController->setRespawnPosition(m_levelData.spawnPoint);
    }

    void loadLevel() {
        if (!m_levelPath.empty()) {
            m_levelData = LevelLoader::load(m_levelPath, m_world);
            if (m_levelData.loaded) return;
        }
        std::string assetsDir = Paths::getAssetPath("");
        m_levelData = LevelLoader::load(assetsDir + "/levels/level_01.json", m_world);
        if (!m_levelData.loaded) {
            spawnHardcodedLevel();
        }
    }

    void countCheckpoints() {
        m_totalCheckpoints = 0;
        m_world.each<CheckpointComponent>(
            [&](Entity, CheckpointComponent&) { m_totalCheckpoints++; }
        );
    }

    void updatePlayerAnimation(float dt) {
        if (!m_world.alive(m_player)) return;
        if (!m_world.has<ModelComponent>(m_player)) return;

        auto& mc = m_world.get<ModelComponent>(m_player);
        auto& pc = m_world.get<PlayerComponent>(m_player);
        auto& rb = m_world.get<RigidBodyComponent>(m_player);
        if (!mc.model || mc.model->clips.empty()) return;

        // Pick clip based on player state
        const char* clipName = "Walk";
        switch (pc.state) {
        case PlayerState::Crouching: clipName = "Crouch"; break;
        case PlayerState::Prone:
        case PlayerState::Sliding:
        case PlayerState::Dead:      clipName = "Prone"; break;
        case PlayerState::Jumping:
        case PlayerState::Falling:   clipName = "Jump"; break;
        case PlayerState::Idle:
        case PlayerState::Running:
        default:                     clipName = "Walk"; break;
        }

        int newClip = mc.model->findClip(clipName);
        if (newClip < 0) newClip = 0;

        if (newClip != mc.currentClip) {
            mc.previousClip = mc.currentClip;
            mc.currentClip = newClip;
            mc.currentTime = 0.0f;
        }

        // Looping rules: only Walk loops; everything else holds its last frame
        std::string name = (mc.currentClip >= 0 && mc.currentClip < (int)mc.model->clips.size())
            ? mc.model->clips[mc.currentClip].name : "";
        mc.looping = (name == "Walk");

        // Slow down animation when standing still (Walk clip)
        float speedScale = 1.0f;
        if (pc.state == PlayerState::Idle || pc.state == PlayerState::Running) {
            float speed = glm::length(glm::vec2(rb.velocity.x, rb.velocity.z));
            speedScale = std::max(0.3f, std::min(speed / 6.0f, 2.0f));
        }

        mc.currentTime += dt * mc.playbackSpeed * speedScale;
    }

    void setupParticleEmitters() {
        m_particles.clearEmitters();

        // Add continuous emitters at all jump pad positions
        m_world.each<TransformComponent, TriggerComponent>(
            [&](Entity, TransformComponent& tc, TriggerComponent& trig) {
                if (trig.type == TriggerType::JumpPad) {
                    ContinuousEmitter em;
                    em.position = tc.position + glm::vec3(0.0f, 0.3f, 0.0f);
                    em.velocityMin = {-0.4f, 1.5f, -0.4f};
                    em.velocityMax = {0.4f, 3.5f, 0.4f};
                    em.colorStart = {1.0f, 0.8f, 0.0f, 0.7f};
                    em.colorEnd = {1.0f, 0.4f, 0.0f, 0.0f};
                    em.sizeStart = 0.12f;
                    em.sizeEnd = 0.03f;
                    em.lifeMin = 0.3f;
                    em.lifeMax = 0.6f;
                    em.gravity = 0.5f;
                    em.spawnRate = 10.0f;
                    m_particles.addEmitter(em);
                }
            }
        );
    }

    void startDeathAnimation() {
        if (m_deathAnimating) return; // Already dying
        m_deathAnimating = true;
        m_deathTimer = 0.0f;
        m_audio.play(SFX::Death);
        // Freeze the player in place
        if (m_world.alive(m_player)) {
            auto& rb = m_world.get<RigidBodyComponent>(m_player);
            rb.velocity = glm::vec3(0.0f);
            rb.isKinematic = true; // Stop physics
            auto& pc = m_world.get<PlayerComponent>(m_player);
            pc.state = PlayerState::Dead;
        }
    }

    void restartLevel() {
        auto ents = m_world.entities();
        for (auto e : ents) {
            m_world.destroy(e);
        }
        m_checkpointSystem.reset();
        m_levelComplete = false;
        m_deathAnimating = false;
        m_deathTimer = 0.0f;
        m_speedBoostTimer = 0.0f;
        loadLevel();
        spawnPlayer();
        countCheckpoints();
        setupParticleEmitters();
        m_levelTimer.start();
    }

    void applyMovingPlatformCarry() {
        if (!m_world.alive(m_player)) return;
        auto& rb = m_world.get<RigidBodyComponent>(m_player);
        if (!rb.isGrounded || !rb.groundEntity.valid()) return;
        if (!m_world.alive(rb.groundEntity)) return;
        if (!m_world.has<ObstacleComponent>(rb.groundEntity)) return;

        auto& obs = m_world.get<ObstacleComponent>(rb.groundEntity);
        if (obs.type != ObstacleType::MovingPlatform) return;

        auto& platTc = m_world.get<TransformComponent>(rb.groundEntity);
        glm::vec3 delta = platTc.position - obs.previousPosition;
        auto& playerTc = m_world.get<TransformComponent>(m_player);
        playerTc.position += delta;
    }

    void spawnHardcodedLevel() {
        m_levelData.name = "Hardcoded Fallback";
        m_levelData.spawnPoint = {0, 3, 0};

        auto spawnPlat = [&](const glm::vec3& pos, const glm::vec3& scale,
                             const glm::vec3& bc, const glm::vec3& ec, float es) {
            Entity e = m_world.create();
            auto& tc = m_world.add<TransformComponent>(e);
            tc.position = pos; tc.scale = scale;
            m_world.add<MeshComponent>(e, {MeshType::Cube});
            m_world.add<MaterialComponent>(e, {bc, ec, es});
            auto& col = m_world.add<ColliderComponent>(e);
            col.type = ColliderType::AABBCollider;
            col.halfExtents = glm::vec3(0.5f);
        };

        spawnPlat({0,0,0}, {6,1,6}, {0.06f,0.06f,0.08f}, {0,0.4f,0.6f}, 0.4f);
        spawnPlat({0,0,8}, {4,1,4}, {0.06f,0.06f,0.08f}, {0,0.6f,0.4f}, 0.5f);
        spawnPlat({4,1,14}, {3,1,3}, {0.08f,0.04f,0.06f}, {0.8f,0,1}, 0.6f);
        spawnPlat({0,2,20}, {3,1,3}, {0.06f,0.08f,0.04f}, {0,1,0.3f}, 0.6f);
        spawnPlat({-4,3,26}, {3,1,3}, {0.08f,0.06f,0.02f}, {1,0.5f,0}, 0.7f);
        spawnPlat({0,3,32}, {3,1,3}, {0.08f,0.02f,0.06f}, {1,0,0.8f}, 0.6f);
        for (int i = 0; i < 5; i++) {
            float y = 4.0f + i * 1.2f;
            float z = 38.0f + i * 4.0f;
            float x = (i % 2 == 0) ? 2.0f : -2.0f;
            spawnPlat({x,y,z}, {2.5f,0.6f,2.5f}, {0.05f,0.05f,0.08f}, {0.2f,0.4f,1}, 0.5f);
        }
        spawnPlat({0,10,58}, {5,1,5}, {0.06f,0.08f,0.06f}, {1,1,0}, 1.0f);
        spawnPlat({0,1,11}, {1.2f,0.5f,6}, {0.08f,0.04f,0.04f}, {1,0.2f,0.2f}, 0.4f);
    }
};
