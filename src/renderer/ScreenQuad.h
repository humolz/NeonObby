#pragma once

#include <glad/glad.h>

class ScreenQuad {
public:
    ScreenQuad() {
        float verts[] = {
            // pos(x,y), texcoord(u,v)
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f,
            -1.0f,  1.0f,  0.0f, 1.0f,
        };
        uint32_t indices[] = {0, 1, 2, 0, 2, 3};

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
            reinterpret_cast<void*>(2 * sizeof(float)));

        glBindVertexArray(0);
    }

    ~ScreenQuad() {
        if (m_ebo) glDeleteBuffers(1, &m_ebo);
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
    }

    ScreenQuad(const ScreenQuad&) = delete;
    ScreenQuad& operator=(const ScreenQuad&) = delete;

    void draw() const {
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

private:
    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
};
