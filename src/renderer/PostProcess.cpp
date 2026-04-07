#include "renderer/PostProcess.h"

void PostProcess::init(int width, int height, const std::string& assetsDir) {
    m_blurShader = std::make_unique<Shader>(
        assetsDir + "/shaders/postprocess.vert",
        assetsDir + "/shaders/bloom_blur.frag"
    );
    m_compositeShader = std::make_unique<Shader>(
        assetsDir + "/shaders/postprocess.vert",
        assetsDir + "/shaders/bloom_composite.frag"
    );
    createFBOs(width, height);
}

void PostProcess::resize(int width, int height) {
    if (width == m_width && height == m_height) return;
    createFBOs(width, height);
}

void PostProcess::createFBOs(int w, int h) {
    m_width = w;
    m_height = h;

    // Scene FBO: 2 color attachments (scene color + bright pass), with depth
    m_sceneFBO.create({w, h, 2, true, true, false});

    // Ping-pong FBOs: half-res, 1 color attachment, no depth
    m_pingFBO.create({w, h, 1, false, true, true});
    m_pongFBO.create({w, h, 1, false, true, true});
}

void PostProcess::beginScene() {
    m_sceneFBO.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcess::endScene(int screenWidth, int screenHeight) {
    // --- Bloom blur pass ---
    glDisable(GL_DEPTH_TEST);

    m_blurShader->bind();
    m_blurShader->setInt("u_image", 0);

    bool horizontal = true;
    // First pass: read from scene bright-pass (attachment 1)
    GLuint inputTex = m_sceneFBO.colorTexture(1);

    for (int i = 0; i < blurPasses * 2; i++) {
        if (horizontal) {
            m_pingFBO.bind();
        } else {
            m_pongFBO.bind();
        }

        m_blurShader->setInt("u_horizontal", horizontal ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTex);

        m_quad.draw();

        // Next pass reads from what we just wrote
        inputTex = horizontal ? m_pingFBO.colorTexture(0) : m_pongFBO.colorTexture(0);
        horizontal = !horizontal;
    }

    // --- Composite pass: blend scene + bloom → screen ---
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    m_compositeShader->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneFBO.colorTexture(0)); // scene color
    m_compositeShader->setInt("u_scene", 0);

    glActiveTexture(GL_TEXTURE1);
    // Final blur result is in the last written FBO
    GLuint bloomTex = (blurPasses * 2 % 2 == 0) ? m_pongFBO.colorTexture(0) : m_pingFBO.colorTexture(0);
    glBindTexture(GL_TEXTURE_2D, bloomTex);
    m_compositeShader->setInt("u_bloom", 1);

    m_compositeShader->setFloat("u_bloomIntensity", bloomIntensity);
    m_compositeShader->setFloat("u_exposure", exposure);

    m_quad.draw();

    glEnable(GL_DEPTH_TEST);
}
