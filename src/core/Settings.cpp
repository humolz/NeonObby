#include "core/Settings.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

using json = nlohmann::json;

namespace Settings {

namespace {
    Data g_data;

    // Return "%APPDATA%\NeonObby\settings.json" on Windows, "~/.neonobby/..."
    // elsewhere. Mirrors SaveManager::getSavePath() so both files live next to
    // each other and the uninstaller leaves them alone.
    std::string getSettingsPath() {
        std::string dir;
#ifdef _WIN32
        char path[MAX_PATH];
        if (SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path) == S_OK) {
            dir = std::string(path) + "\\NeonObby";
        } else {
            dir = ".";
        }
        CreateDirectoryA(dir.c_str(), nullptr);
#else
        const char* home = std::getenv("HOME");
        dir = home ? std::string(home) + "/.neonobby" : ".";
        std::filesystem::create_directories(dir);
#endif
        return dir + "/settings.json";
    }
} // namespace

Data& get() { return g_data; }

void load() {
    std::ifstream f(getSettingsPath());
    if (!f.is_open()) return; // first run → defaults

    try {
        json j;
        f >> j;

        // Every field is defensive — a missing key leaves the default in place.
        // This keeps older settings files forward-compatible when we add new
        // options in a later version.
        if (j.contains("fov"))            g_data.fov          = j["fov"].get<float>();
        if (j.contains("mouseSens"))      g_data.mouseSens    = j["mouseSens"].get<float>();
        if (j.contains("invertY"))        g_data.invertY      = j["invertY"].get<bool>();
        if (j.contains("masterVolume"))   g_data.masterVolume = j["masterVolume"].get<float>();
        if (j.contains("displayMode"))    g_data.displayMode  = static_cast<DisplayMode>(j["displayMode"].get<int>());
        if (j.contains("vsync"))          g_data.vsync        = j["vsync"].get<bool>();
        if (j.contains("windowedWidth"))  g_data.windowedWidth  = j["windowedWidth"].get<int>();
        if (j.contains("windowedHeight")) g_data.windowedHeight = j["windowedHeight"].get<int>();
        if (j.contains("showFps"))        g_data.showFps      = j["showFps"].get<bool>();
        if (j.contains("showControlsHint")) g_data.showControlsHint = j["showControlsHint"].get<bool>();
        if (j.contains("bloomIntensity")) g_data.bloomIntensity = j["bloomIntensity"].get<float>();

        if (j.contains("keys")) {
            const auto& k = j["keys"];
            auto readKey = [&](const char* name, int& dst) {
                if (k.contains(name)) dst = k[name].get<int>();
            };
            readKey("moveForward", g_data.keys.moveForward);
            readKey("moveBack",    g_data.keys.moveBack);
            readKey("moveLeft",    g_data.keys.moveLeft);
            readKey("moveRight",   g_data.keys.moveRight);
            readKey("jump",        g_data.keys.jump);
            readKey("crouch",      g_data.keys.crouch);
            readKey("prone",       g_data.keys.prone);
            readKey("pause",       g_data.keys.pause);
            readKey("restart",     g_data.keys.restart);
            readKey("debug",       g_data.keys.debug);
            readKey("freeCursor",  g_data.keys.freeCursor);
        }
    } catch (...) {
        // Corrupt JSON — fall back to defaults silently. We don't want a bad
        // settings file to block the game from starting.
    }
}

void save() {
    json j;
    j["fov"]            = g_data.fov;
    j["mouseSens"]      = g_data.mouseSens;
    j["invertY"]        = g_data.invertY;
    j["masterVolume"]   = g_data.masterVolume;
    j["displayMode"]    = static_cast<int>(g_data.displayMode);
    j["vsync"]          = g_data.vsync;
    j["windowedWidth"]  = g_data.windowedWidth;
    j["windowedHeight"] = g_data.windowedHeight;
    j["showFps"]        = g_data.showFps;
    j["showControlsHint"] = g_data.showControlsHint;
    j["bloomIntensity"] = g_data.bloomIntensity;

    json k;
    k["moveForward"] = g_data.keys.moveForward;
    k["moveBack"]    = g_data.keys.moveBack;
    k["moveLeft"]    = g_data.keys.moveLeft;
    k["moveRight"]   = g_data.keys.moveRight;
    k["jump"]        = g_data.keys.jump;
    k["crouch"]      = g_data.keys.crouch;
    k["prone"]       = g_data.keys.prone;
    k["pause"]       = g_data.keys.pause;
    k["restart"]     = g_data.keys.restart;
    k["debug"]       = g_data.keys.debug;
    k["freeCursor"]  = g_data.keys.freeCursor;
    j["keys"] = k;

    std::ofstream f(getSettingsPath());
    if (f.is_open()) f << j.dump(2);
}

std::string keyName(int glfwKey) {
    // GLFW exposes printable-key names via glfwGetKeyName() (returns "a" etc.
    // when the keyboard layout is loaded), but that function doesn't cover
    // special keys. We ship our own table for the non-printable ones and fall
    // back to glfwGetKeyName() for everything else.
    switch (glfwKey) {
    case GLFW_KEY_SPACE:         return "Space";
    case GLFW_KEY_APOSTROPHE:    return "'";
    case GLFW_KEY_COMMA:         return ",";
    case GLFW_KEY_MINUS:         return "-";
    case GLFW_KEY_PERIOD:        return ".";
    case GLFW_KEY_SLASH:         return "/";
    case GLFW_KEY_SEMICOLON:     return ";";
    case GLFW_KEY_EQUAL:         return "=";
    case GLFW_KEY_LEFT_BRACKET:  return "[";
    case GLFW_KEY_BACKSLASH:     return "\\";
    case GLFW_KEY_RIGHT_BRACKET: return "]";
    case GLFW_KEY_GRAVE_ACCENT:  return "`";
    case GLFW_KEY_ESCAPE:        return "Esc";
    case GLFW_KEY_ENTER:         return "Enter";
    case GLFW_KEY_TAB:           return "Tab";
    case GLFW_KEY_BACKSPACE:     return "Backspace";
    case GLFW_KEY_INSERT:        return "Insert";
    case GLFW_KEY_DELETE:        return "Delete";
    case GLFW_KEY_RIGHT:         return "Right";
    case GLFW_KEY_LEFT:          return "Left";
    case GLFW_KEY_DOWN:          return "Down";
    case GLFW_KEY_UP:            return "Up";
    case GLFW_KEY_PAGE_UP:       return "PgUp";
    case GLFW_KEY_PAGE_DOWN:     return "PgDn";
    case GLFW_KEY_HOME:          return "Home";
    case GLFW_KEY_END:           return "End";
    case GLFW_KEY_CAPS_LOCK:     return "CapsLock";
    case GLFW_KEY_SCROLL_LOCK:   return "ScrollLock";
    case GLFW_KEY_NUM_LOCK:      return "NumLock";
    case GLFW_KEY_PRINT_SCREEN:  return "PrtSc";
    case GLFW_KEY_PAUSE:         return "Pause";
    case GLFW_KEY_LEFT_SHIFT:    return "Left Shift";
    case GLFW_KEY_LEFT_CONTROL:  return "Left Ctrl";
    case GLFW_KEY_LEFT_ALT:      return "Left Alt";
    case GLFW_KEY_LEFT_SUPER:    return "Left Win";
    case GLFW_KEY_RIGHT_SHIFT:   return "Right Shift";
    case GLFW_KEY_RIGHT_CONTROL: return "Right Ctrl";
    case GLFW_KEY_RIGHT_ALT:     return "Right Alt";
    case GLFW_KEY_RIGHT_SUPER:   return "Right Win";
    case GLFW_KEY_MENU:          return "Menu";
    default: break;
    }
    if (glfwKey >= GLFW_KEY_F1 && glfwKey <= GLFW_KEY_F25) {
        return "F" + std::to_string(glfwKey - GLFW_KEY_F1 + 1);
    }
    if (glfwKey >= GLFW_KEY_KP_0 && glfwKey <= GLFW_KEY_KP_9) {
        return "KP " + std::to_string(glfwKey - GLFW_KEY_KP_0);
    }
    const char* name = glfwGetKeyName(glfwKey, 0);
    if (name && name[0]) {
        std::string s(name);
        // Upper-case printable letter keys for readability
        if (s.size() == 1 && s[0] >= 'a' && s[0] <= 'z') {
            s[0] = static_cast<char>(s[0] - 'a' + 'A');
        }
        return s;
    }
    return "Key " + std::to_string(glfwKey);
}

} // namespace Settings
