#pragma once

#include "renderer/Framebuffer.h"
#include "renderer/Shader.h"
#include "renderer/ScreenQuad.h"
#include <memory>

class PostProcess {
public:
    PostProcess() = default;

    void init(int width, int height, const std::string& assetsDir);
    void resize(int width, int height);

    // Call before rendering scene geometry
    void beginScene();
    // Call after rendering scene geometry — runs bloom and composites to screen
    void endScene(int screenWidth, int screenHeight);

    float bloomIntensity = 0.7f;
    float exposure = 0.9f;
    int blurPasses = 5;

private:
    Framebuffer m_sceneFBO;    // 2 color attachments: scene + bright-pass
    Framebuffer m_pingFBO;     // half-res, single attachment
    Framebuffer m_pongFBO;     // half-res, single attachment

    std::unique_ptr<Shader> m_blurShader;
    std::unique_ptr<Shader> m_compositeShader;
    ScreenQuad m_quad;

    int m_width = 0, m_height = 0;

    void createFBOs(int w, int h);
};
