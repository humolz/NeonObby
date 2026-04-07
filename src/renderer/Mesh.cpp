#include "renderer/Mesh.h"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : m_indexCount(static_cast<uint32_t>(indices.size()))
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
        indices.data(), GL_STATIC_DRAW);

    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, position)));

    // Normal attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, normal)));

    glBindVertexArray(0);
}

Mesh::~Mesh() {
    cleanup();
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo), m_indexCount(other.m_indexCount)
{
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
    other.m_indexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

void Mesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::cleanup() {
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}
