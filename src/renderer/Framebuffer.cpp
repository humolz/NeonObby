#include "renderer/Framebuffer.h"
#include <stdexcept>
#include <cstdio>

Framebuffer::~Framebuffer() {
    destroy();
}

Framebuffer::Framebuffer(Framebuffer&& o) noexcept
    : m_fbo(o.m_fbo), m_depthRbo(o.m_depthRbo),
      m_colorTextures(std::move(o.m_colorTextures)),
      m_width(o.m_width), m_height(o.m_height)
{
    o.m_fbo = 0;
    o.m_depthRbo = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& o) noexcept {
    if (this != &o) {
        destroy();
        m_fbo = o.m_fbo;
        m_depthRbo = o.m_depthRbo;
        m_colorTextures = std::move(o.m_colorTextures);
        m_width = o.m_width;
        m_height = o.m_height;
        o.m_fbo = 0;
        o.m_depthRbo = 0;
    }
    return *this;
}

void Framebuffer::create(const Config& cfg) {
    destroy();

    m_width = cfg.halfRes ? cfg.width / 2 : cfg.width;
    m_height = cfg.halfRes ? cfg.height / 2 : cfg.height;
    if (m_width < 1) m_width = 1;
    if (m_height < 1) m_height = 1;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    GLenum internalFmt = cfg.hdr ? GL_RGBA16F : GL_RGBA8;
    GLenum dataType = cfg.hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;

    m_colorTextures.resize(cfg.colorAttachments);
    std::vector<GLenum> drawBuffers;

    for (int i = 0; i < cfg.colorAttachments; i++) {
        glGenTextures(1, &m_colorTextures[i]);
        glBindTexture(GL_TEXTURE_2D, m_colorTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, m_width, m_height,
                     0, GL_RGBA, dataType, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D, m_colorTextures[i], 0);
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }

    glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());

    if (cfg.useDepth) {
        glGenRenderbuffers(1, &m_depthRbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, m_depthRbo);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::printf("Framebuffer incomplete!\n");
        throw std::runtime_error("Framebuffer not complete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::destroy() {
    for (auto tex : m_colorTextures) {
        if (tex) glDeleteTextures(1, &tex);
    }
    m_colorTextures.clear();
    if (m_depthRbo) { glDeleteRenderbuffers(1, &m_depthRbo); m_depthRbo = 0; }
    if (m_fbo) { glDeleteFramebuffers(1, &m_fbo); m_fbo = 0; }
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void Framebuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
