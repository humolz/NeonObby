#pragma once

#include <glad/glad.h>
#include <vector>

class Framebuffer {
public:
    struct Config {
        int width;
        int height;
        int colorAttachments = 1;   // number of color textures
        bool useDepth = true;
        bool hdr = true;            // RGBA16F vs RGBA8
        bool halfRes = false;       // create at half resolution
    };

    Framebuffer() = default;
    ~Framebuffer();

    Framebuffer(Framebuffer&& o) noexcept;
    Framebuffer& operator=(Framebuffer&& o) noexcept;
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void create(const Config& cfg);
    void destroy();
    void bind() const;
    void unbind() const;

    GLuint colorTexture(int index = 0) const { return m_colorTextures[index]; }
    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    GLuint m_fbo = 0;
    GLuint m_depthRbo = 0;
    std::vector<GLuint> m_colorTextures;
    int m_width = 0;
    int m_height = 0;
};
