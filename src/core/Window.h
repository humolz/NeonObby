#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

#include "core/Settings.h"

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();

    GLFWwindow* handle() const { return m_window; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    float aspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }

    // Switch between windowed, exclusive fullscreen, and borderless-windowed
    // at runtime. Uses glfwSetWindowMonitor under the hood — no context
    // recreation, so OpenGL resources stay valid across the change.
    void applyDisplayMode(Settings::DisplayMode mode);

    // glfwSwapInterval wrapper. Applied before the first swap so the change
    // takes effect immediately.
    void setVSync(bool enabled);

    using ResizeCallback = std::function<void(int, int)>;
    void setResizeCallback(ResizeCallback cb) { m_resizeCallback = cb; }

private:
    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    // Remember the last windowed-mode size/position so we can restore it
    // after leaving fullscreen.
    int m_windowedX = 100;
    int m_windowedY = 100;
    int m_windowedW = 1280;
    int m_windowedH = 720;
    Settings::DisplayMode m_mode = Settings::DisplayMode::Windowed;
    ResizeCallback m_resizeCallback;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};
