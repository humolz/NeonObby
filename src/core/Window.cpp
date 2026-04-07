#include "core/Window.h"
#include <stdexcept>
#include <cstdio>

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height),
      m_windowedW(width), m_windowedH(height)
{
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Record the windowed-mode origin so applyDisplayMode() can put the
    // window back here after the user exits fullscreen.
    glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glViewport(0, 0, width, height);

    std::printf("OpenGL %s | GLSL %s\n",
        glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
}

Window::~Window() {
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_width = width;
    self->m_height = height;
    glViewport(0, 0, width, height);
    if (self->m_resizeCallback) {
        self->m_resizeCallback(width, height);
    }
}

void Window::applyDisplayMode(Settings::DisplayMode mode) {
    if (mode == m_mode) return; // no-op if already in this mode

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vid = monitor ? glfwGetVideoMode(monitor) : nullptr;
    if (!vid) return;

    switch (mode) {
    case Settings::DisplayMode::Windowed: {
        // Restore chrome and put the window back where the user had it.
        glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(m_window, nullptr,
            m_windowedX, m_windowedY, m_windowedW, m_windowedH, GLFW_DONT_CARE);
        break;
    }
    case Settings::DisplayMode::Fullscreen: {
        // Save the current windowed position so we can come back to it.
        if (m_mode == Settings::DisplayMode::Windowed) {
            glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
            m_windowedW = m_width;
            m_windowedH = m_height;
        }
        glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(m_window, monitor, 0, 0,
                             vid->width, vid->height, vid->refreshRate);
        break;
    }
    case Settings::DisplayMode::BorderlessWindowed: {
        // Save the current windowed position if we're coming from windowed.
        if (m_mode == Settings::DisplayMode::Windowed) {
            glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
            m_windowedW = m_width;
            m_windowedH = m_height;
        }
        // Borderless = windowed at monitor size with no chrome. This is the
        // "fake fullscreen" mode most modern games default to — Alt-Tab is
        // instant, no mode switch, no display flicker.
        glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(m_window, nullptr, 0, 0,
                             vid->width, vid->height, GLFW_DONT_CARE);
        break;
    }
    }
    m_mode = mode;
}

void Window::setVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
}
