#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

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

    using ResizeCallback = std::function<void(int, int)>;
    void setResizeCallback(ResizeCallback cb) { m_resizeCallback = cb; }

private:
    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    ResizeCallback m_resizeCallback;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};
