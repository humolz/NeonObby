#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "core/Settings.h"

class Input {
public:
    static void init(GLFWwindow* window) {
        s_window = window;
        glfwSetCursorPosCallback(window, cursorCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
    }

    static void update() {
        s_mouseDelta = {0.0f, 0.0f};
        s_scrollDelta = 0.0f;
        glfwPollEvents();
    }

    // After pollEvents, GLFW callbacks have set the deltas
    static void postUpdate() {
        // nothing needed — deltas are set in callbacks
    }

    static GLFWwindow* window() { return s_window; }
    static bool keyDown(int key) { return glfwGetKey(s_window, key) == GLFW_PRESS; }
    static bool mouseDown(int button) { return glfwGetMouseButton(s_window, button) == GLFW_PRESS; }

    static glm::vec2 mouseDelta() { return s_mouseDelta; }
    static float scrollDelta() { return s_scrollDelta; }

    // Movement vector from the user's bound keys (normalized if nonzero).
    // Reads live from Settings so remapping in the pause menu takes effect
    // without restarting the level.
    static glm::vec2 moveInput() {
        const auto& k = Settings::get().keys;
        glm::vec2 dir(0.0f);
        if (keyDown(k.moveForward)) dir.y += 1.0f;
        if (keyDown(k.moveBack))    dir.y -= 1.0f;
        if (keyDown(k.moveLeft))    dir.x -= 1.0f;
        if (keyDown(k.moveRight))   dir.x += 1.0f;
        if (glm::length(dir) > 0.0f) dir = glm::normalize(dir);
        return dir;
    }

private:
    static inline GLFWwindow* s_window = nullptr;
    static inline glm::vec2 s_mouseDelta{0.0f};
    static inline float s_scrollDelta = 0.0f;
    static inline double s_lastX = 0.0, s_lastY = 0.0;
    static inline bool s_firstMouse = true;

    static void cursorCallback(GLFWwindow*, double x, double y) {
        if (s_firstMouse) {
            s_lastX = x; s_lastY = y; s_firstMouse = false;
        }
        s_mouseDelta.x += static_cast<float>(x - s_lastX);
        s_mouseDelta.y += static_cast<float>(y - s_lastY);
        s_lastX = x;
        s_lastY = y;
    }

    static void scrollCallback(GLFWwindow*, double, double yoff) {
        s_scrollDelta += static_cast<float>(yoff);
    }

    static void mouseButtonCallback(GLFWwindow*, int, int, int) {
        // available for future use
    }
};
