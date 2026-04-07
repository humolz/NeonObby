#pragma once

#include <GLFW/glfw3.h>

class Timer {
public:
    void tick() {
        double now = glfwGetTime();
        m_dt = static_cast<float>(now - m_lastTime);
        if (m_dt > 0.25f) m_dt = 0.25f;
        m_lastTime = now;
        m_elapsed += m_dt;
        m_frameCount++;
    }

    float deltaTime() const { return m_dt; }
    float elapsed() const { return m_elapsed; }
    int frameCount() const { return m_frameCount; }

    // Returns FPS, resets counter each second
    int fps() {
        m_fpsAccum += m_dt;
        m_fpsFrames++;
        if (m_fpsAccum >= 1.0f) {
            m_lastFps = m_fpsFrames;
            m_fpsFrames = 0;
            m_fpsAccum = 0.0f;
        }
        return m_lastFps;
    }

private:
    double m_lastTime = glfwGetTime();
    float m_dt = 0.0f;
    float m_elapsed = 0.0f;
    int m_frameCount = 0;
    float m_fpsAccum = 0.0f;
    int m_fpsFrames = 0;
    int m_lastFps = 0;
};
