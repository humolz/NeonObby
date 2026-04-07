#pragma once

class TimerSystem {
public:
    void update(float dt) {
        if (m_running) {
            m_elapsed += dt;
        }
    }

    void start() { m_running = true; m_elapsed = 0.0f; }
    void stop() { m_running = false; }
    void pause() { m_running = false; }
    void resume() { m_running = true; }
    void reset() { m_elapsed = 0.0f; m_running = false; }

    float elapsed() const { return m_elapsed; }
    bool running() const { return m_running; }

    int minutes() const { return static_cast<int>(m_elapsed) / 60; }
    int seconds() const { return static_cast<int>(m_elapsed) % 60; }
    int milliseconds() const { return static_cast<int>((m_elapsed - static_cast<int>(m_elapsed)) * 1000); }

private:
    float m_elapsed = 0.0f;
    bool m_running = false;
};
