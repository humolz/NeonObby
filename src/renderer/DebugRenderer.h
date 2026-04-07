#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "physics/AABB.h"
#include <vector>

struct DebugVertex {
    glm::vec3 position;
    glm::vec3 color;
};

class DebugRenderer {
public:
    void init() {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTS * sizeof(DebugVertex), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex),
                              (void*)offsetof(DebugVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex),
                              (void*)offsetof(DebugVertex, color));

        glBindVertexArray(0);
    }

    void drawAABB(const AABB& box, const glm::vec3& color) {
        glm::vec3 lo = box.min;
        glm::vec3 hi = box.max;

        // 8 corners
        glm::vec3 c[8] = {
            {lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z},
            {hi.x, hi.y, lo.z}, {lo.x, hi.y, lo.z},
            {lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z},
            {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z},
        };

        // 12 edges (as line pairs)
        auto line = [&](int a, int b) {
            m_vertices.push_back({c[a], color});
            m_vertices.push_back({c[b], color});
        };

        // Bottom face
        line(0, 1); line(1, 2); line(2, 3); line(3, 0);
        // Top face
        line(4, 5); line(5, 6); line(6, 7); line(7, 4);
        // Vertical edges
        line(0, 4); line(1, 5); line(2, 6); line(3, 7);
    }

    void drawCapsule(const glm::vec3& pos, float halfHeight, float radius, const glm::vec3& color) {
        // Approximate capsule as a box + circles
        glm::vec3 lo = pos - glm::vec3(radius, halfHeight + radius, radius);
        glm::vec3 hi = pos + glm::vec3(radius, halfHeight + radius, radius);
        AABB box{lo, hi};
        drawAABB(box, color);
    }

    void flush(const glm::mat4& /*viewProj*/) {
        if (m_vertices.empty()) return;

        int count = static_cast<int>(m_vertices.size());
        if (count > MAX_VERTS) count = MAX_VERTS;

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(DebugVertex), m_vertices.data());

        glBindVertexArray(m_vao);
        glDrawArrays(GL_LINES, 0, count);
        glBindVertexArray(0);

        m_vertices.clear();
    }

    bool enabled = false;

private:
    static constexpr int MAX_VERTS = 16384;
    GLuint m_vao = 0, m_vbo = 0;
    std::vector<DebugVertex> m_vertices;
};
