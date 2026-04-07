#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib>

struct Particle {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec4 colorStart{1.0f};
    glm::vec4 colorEnd{1.0f, 1.0f, 1.0f, 0.0f};
    float size = 0.1f;
    float sizeEnd = 0.0f;
    float life = 0.0f;
    float maxLife = 1.0f;
    bool alive = false;
};

struct BurstConfig {
    glm::vec3 position{0.0f};
    int count = 30;
    glm::vec3 velocityMin{-1.0f, 1.0f, -1.0f};
    glm::vec3 velocityMax{1.0f, 4.0f, 1.0f};
    glm::vec4 colorStart{0.0f, 0.8f, 1.0f, 1.0f};
    glm::vec4 colorEnd{0.0f, 0.3f, 1.0f, 0.0f};
    float sizeStart = 0.15f;
    float sizeEnd = 0.02f;
    float lifeMin = 0.8f;
    float lifeMax = 1.5f;
    float gravity = -2.0f;
};

struct ContinuousEmitter {
    glm::vec3 position{0.0f};
    glm::vec3 velocityMin{-0.3f, 2.0f, -0.3f};
    glm::vec3 velocityMax{0.3f, 4.0f, 0.3f};
    glm::vec4 colorStart{1.0f, 0.8f, 0.0f, 0.8f};
    glm::vec4 colorEnd{1.0f, 0.5f, 0.0f, 0.0f};
    float sizeStart = 0.12f;
    float sizeEnd = 0.03f;
    float lifeMin = 0.3f;
    float lifeMax = 0.7f;
    float gravity = -1.0f;
    float spawnRate = 8.0f;  // particles per second
    float accumulator = 0.0f;
    bool active = true;
};

// GPU instance data layout
struct ParticleInstance {
    glm::vec3 position;
    glm::vec4 color;
    float size;
};

class ParticleSystem {
public:
    void init(int maxParticles = 2048) {
        m_maxParticles = maxParticles;
        m_particles.resize(maxParticles);
        m_instances.reserve(maxParticles);

        // Unit quad vertices: 4 corners
        float quadVerts[] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
             0.5f,  0.5f,
            -0.5f,  0.5f,
        };
        unsigned int quadIndices[] = { 0, 1, 2, 0, 2, 3 };

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_quadVBO);
        glGenBuffers(1, &m_ebo);
        glGenBuffers(1, &m_instanceVBO);

        glBindVertexArray(m_vao);

        // Quad geometry
        glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

        // Index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

        // Instance buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(ParticleInstance), nullptr, GL_DYNAMIC_DRAW);

        // location 1: position (vec3)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance),
                              (void*)offsetof(ParticleInstance, position));
        glVertexAttribDivisor(1, 1);

        // location 2: color (vec4)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance),
                              (void*)offsetof(ParticleInstance, color));
        glVertexAttribDivisor(2, 1);

        // location 3: size (float)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance),
                              (void*)offsetof(ParticleInstance, size));
        glVertexAttribDivisor(3, 1);

        glBindVertexArray(0);
    }

    void spawnBurst(const BurstConfig& cfg) {
        for (int i = 0; i < cfg.count; i++) {
            Particle* p = findDeadParticle();
            if (!p) break;

            p->alive = true;
            p->position = cfg.position;
            p->velocity = randomVec3(cfg.velocityMin, cfg.velocityMax);
            p->colorStart = cfg.colorStart;
            p->colorEnd = cfg.colorEnd;
            p->size = cfg.sizeStart;
            p->sizeEnd = cfg.sizeEnd;
            p->maxLife = randomFloat(cfg.lifeMin, cfg.lifeMax);
            p->life = p->maxLife;
        }
        // Store gravity for burst particles
        m_burstGravity = cfg.gravity;
    }

    int addEmitter(const ContinuousEmitter& emitter) {
        m_emitters.push_back(emitter);
        return static_cast<int>(m_emitters.size()) - 1;
    }

    void clearEmitters() {
        m_emitters.clear();
    }

    void update(float dt) {
        // Spawn from continuous emitters
        for (auto& em : m_emitters) {
            if (!em.active) continue;
            em.accumulator += dt;
            float interval = 1.0f / em.spawnRate;
            while (em.accumulator >= interval) {
                em.accumulator -= interval;
                Particle* p = findDeadParticle();
                if (!p) break;

                p->alive = true;
                p->position = em.position;
                p->velocity = randomVec3(em.velocityMin, em.velocityMax);
                p->colorStart = em.colorStart;
                p->colorEnd = em.colorEnd;
                p->size = em.sizeStart;
                p->sizeEnd = em.sizeEnd;
                p->maxLife = randomFloat(em.lifeMin, em.lifeMax);
                p->life = p->maxLife;
            }
        }

        // Update all alive particles
        for (auto& p : m_particles) {
            if (!p.alive) continue;

            p.life -= dt;
            if (p.life <= 0.0f) {
                p.alive = false;
                continue;
            }

            // Simple gravity
            p.velocity.y += m_burstGravity * dt;
            p.position += p.velocity * dt;
        }
    }

    void render(const glm::mat4& /*viewProj*/, const glm::vec3& /*cameraRight*/, const glm::vec3& /*cameraUp*/) {
        // Build instance data from alive particles
        m_instances.clear();
        for (auto& p : m_particles) {
            if (!p.alive) continue;

            float t = 1.0f - (p.life / p.maxLife); // 0 at birth, 1 at death
            glm::vec4 color = glm::mix(p.colorStart, p.colorEnd, t);
            float size = glm::mix(p.size, p.sizeEnd, t);

            m_instances.push_back({p.position, color, size});
        }

        if (m_instances.empty()) return;

        int count = std::min(static_cast<int>(m_instances.size()), m_maxParticles);

        // Upload instance data
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(ParticleInstance), m_instances.data());

        // Render with blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for glow
        glDepthMask(GL_FALSE);

        glBindVertexArray(m_vao);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, count);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    int aliveCount() const {
        int count = 0;
        for (auto& p : m_particles) {
            if (p.alive) count++;
        }
        return count;
    }

private:
    int m_maxParticles = 2048;
    float m_burstGravity = -2.0f;
    std::vector<Particle> m_particles;
    std::vector<ParticleInstance> m_instances;
    std::vector<ContinuousEmitter> m_emitters;
    GLuint m_vao = 0, m_quadVBO = 0, m_ebo = 0, m_instanceVBO = 0;

    Particle* findDeadParticle() {
        for (auto& p : m_particles) {
            if (!p.alive) return &p;
        }
        return nullptr;
    }

    static float randomFloat(float lo, float hi) {
        return lo + static_cast<float>(std::rand()) / RAND_MAX * (hi - lo);
    }

    static glm::vec3 randomVec3(const glm::vec3& lo, const glm::vec3& hi) {
        return {randomFloat(lo.x, hi.x), randomFloat(lo.y, hi.y), randomFloat(lo.z, hi.z)};
    }
};
