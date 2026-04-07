#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <memory>
#include <cmath>

struct SkinnedVertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::ivec4 jointIndices{0};
    glm::vec4 jointWeights{0.0f};
};

struct Bone {
    std::string name;
    int parentIndex = -1;          // -1 if root
    glm::mat4 inverseBindMatrix{1.0f};
    // Bind pose local TRS
    glm::vec3 localTranslation{0.0f};
    glm::quat localRotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 localScale{1.0f};
};

enum class ChannelPath { Translation, Rotation, Scale };

struct AnimationChannel {
    int boneIndex = 0;
    ChannelPath path = ChannelPath::Translation;
    std::vector<float> times;
    std::vector<glm::vec4> values; // xyz for T/S, xyzw quat for R
};

struct AnimationClip {
    std::string name;
    float duration = 0.0f;
    std::vector<AnimationChannel> channels;
};

class SkinnedMesh {
public:
    SkinnedMesh() = default;
    ~SkinnedMesh() {
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        if (m_ebo) glDeleteBuffers(1, &m_ebo);
    }

    SkinnedMesh(const SkinnedMesh&) = delete;
    SkinnedMesh& operator=(const SkinnedMesh&) = delete;

    SkinnedMesh(SkinnedMesh&& o) noexcept
        : m_vao(o.m_vao), m_vbo(o.m_vbo), m_ebo(o.m_ebo), m_indexCount(o.m_indexCount) {
        o.m_vao = o.m_vbo = o.m_ebo = 0;
        o.m_indexCount = 0;
    }

    void upload(const std::vector<SkinnedVertex>& vertices, const std::vector<uint32_t>& indices) {
        m_indexCount = static_cast<int>(indices.size());

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SkinnedVertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                              (void*)offsetof(SkinnedVertex, position));
        // normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                              (void*)offsetof(SkinnedVertex, normal));
        // joints (integer attribute!)
        glEnableVertexAttribArray(2);
        glVertexAttribIPointer(2, 4, GL_INT, sizeof(SkinnedVertex),
                               (void*)offsetof(SkinnedVertex, jointIndices));
        // weights
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                              (void*)offsetof(SkinnedVertex, jointWeights));

        glBindVertexArray(0);
    }

    void draw() const {
        if (!m_vao) return;
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    int indexCount() const { return m_indexCount; }

private:
    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
    int m_indexCount = 0;
};

class Model {
public:
    SkinnedMesh mesh;
    std::vector<Bone> bones;
    std::vector<AnimationClip> clips;

    int findClip(const std::string& name) const {
        for (size_t i = 0; i < clips.size(); i++) {
            if (clips[i].name == name) return static_cast<int>(i);
        }
        return -1;
    }

    // Sample animation and compute final bone matrices.
    // looping=true repeats the clip; looping=false clamps the last frame (hold pose).
    void computeBoneMatrices(int clipIndex, float time, bool looping,
                             std::vector<glm::mat4>& outMatrices) const {
        size_t boneCount = bones.size();
        outMatrices.resize(boneCount);

        // Start with bind pose local TRS
        std::vector<glm::vec3> localT(boneCount);
        std::vector<glm::quat> localR(boneCount);
        std::vector<glm::vec3> localS(boneCount);
        for (size_t i = 0; i < boneCount; i++) {
            localT[i] = bones[i].localTranslation;
            localR[i] = bones[i].localRotation;
            localS[i] = bones[i].localScale;
        }

        // Apply animation channels at the given time
        if (clipIndex >= 0 && clipIndex < static_cast<int>(clips.size())) {
            const AnimationClip& clip = clips[clipIndex];
            float t = 0.0f;
            if (clip.duration > 0.0f) {
                t = looping ? std::fmod(time, clip.duration)
                            : std::min(time, clip.duration);
            }
            for (const auto& ch : clip.channels) {
                if (ch.boneIndex < 0 || ch.boneIndex >= static_cast<int>(boneCount)) continue;
                if (ch.times.empty()) continue;

                // Find keyframe interval
                int prev = 0, next = 0;
                float lerpT = 0.0f;
                if (t <= ch.times.front()) {
                    prev = next = 0;
                } else if (t >= ch.times.back()) {
                    prev = next = static_cast<int>(ch.times.size()) - 1;
                } else {
                    for (size_t i = 0; i < ch.times.size() - 1; i++) {
                        if (t >= ch.times[i] && t < ch.times[i + 1]) {
                            prev = static_cast<int>(i);
                            next = static_cast<int>(i + 1);
                            float dur = ch.times[next] - ch.times[prev];
                            lerpT = dur > 0.0f ? (t - ch.times[prev]) / dur : 0.0f;
                            break;
                        }
                    }
                }

                if (ch.path == ChannelPath::Translation) {
                    glm::vec3 a = glm::vec3(ch.values[prev]);
                    glm::vec3 b = glm::vec3(ch.values[next]);
                    localT[ch.boneIndex] = glm::mix(a, b, lerpT);
                } else if (ch.path == ChannelPath::Rotation) {
                    glm::quat a(ch.values[prev].w, ch.values[prev].x, ch.values[prev].y, ch.values[prev].z);
                    glm::quat b(ch.values[next].w, ch.values[next].x, ch.values[next].y, ch.values[next].z);
                    localR[ch.boneIndex] = glm::slerp(a, b, lerpT);
                } else if (ch.path == ChannelPath::Scale) {
                    glm::vec3 a = glm::vec3(ch.values[prev]);
                    glm::vec3 b = glm::vec3(ch.values[next]);
                    localS[ch.boneIndex] = glm::mix(a, b, lerpT);
                }
            }
        }

        // Compute global transforms walking the hierarchy
        std::vector<glm::mat4> globals(boneCount);
        for (size_t i = 0; i < boneCount; i++) {
            glm::mat4 local = glm::translate(glm::mat4(1.0f), localT[i])
                            * glm::toMat4(localR[i])
                            * glm::scale(glm::mat4(1.0f), localS[i]);

            int parent = bones[i].parentIndex;
            if (parent >= 0 && parent < static_cast<int>(i)) {
                globals[i] = globals[parent] * local;
            } else {
                globals[i] = local;
            }
        }

        // Final bone matrix = global * inverseBind
        for (size_t i = 0; i < boneCount; i++) {
            outMatrices[i] = globals[i] * bones[i].inverseBindMatrix;
        }
    }
};
