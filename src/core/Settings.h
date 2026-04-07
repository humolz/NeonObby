#pragma once

// -----------------------------------------------------------------------------
// User-facing game settings — persisted alongside save.json in %APPDATA%.
//
// All mutable state lives inside a single Settings::Data struct accessed via
// Settings::get(). Subsystems that want a setting just read it when they need
// it; the UI writes to it directly. Call Settings::save() when the user leaves
// the settings screen (or at shutdown) to flush to disk.
//
// Settings are a process-wide singleton — every scene, the input layer, the
// audio engine, and the camera all share the same instance, so toggling the
// FOV slider in the pause menu takes effect on the very next frame.
// -----------------------------------------------------------------------------

#include <GLFW/glfw3.h>
#include <string>

namespace Settings {

// Display / window mode — three canonical options users expect:
//   Windowed           → movable, resizable OS window
//   Fullscreen         → exclusive fullscreen at the monitor's native res
//   BorderlessWindowed → window sized to the monitor with no chrome (Alt-Tab
//                        friendly, no mode switch, most AAA games default here)
enum class DisplayMode : int {
    Windowed = 0,
    Fullscreen = 1,
    BorderlessWindowed = 2
};

// Every remappable game action. Stored as GLFW_KEY_* constants so Input just
// calls glfwGetKey() with whatever value lives here.
struct Keybinds {
    int moveForward = GLFW_KEY_W;
    int moveBack    = GLFW_KEY_S;
    int moveLeft    = GLFW_KEY_A;
    int moveRight   = GLFW_KEY_D;
    int jump        = GLFW_KEY_SPACE;
    int crouch      = GLFW_KEY_LEFT_CONTROL;
    int prone       = GLFW_KEY_Z;
    int pause       = GLFW_KEY_ESCAPE;
    int restart     = GLFW_KEY_R;
    int debug       = GLFW_KEY_F3;
    int freeCursor  = GLFW_KEY_TAB;
};

struct Data {
    // --- Gameplay / camera ---
    float fov          = 60.0f;    // 40..110
    float mouseSens    = 0.15f;    // 0.05..1.0
    bool  invertY      = false;

    // --- Audio ---
    float masterVolume = 0.8f;     // 0..1

    // --- Display ---
    DisplayMode displayMode = DisplayMode::Windowed;
    bool  vsync            = true;
    int   windowedWidth    = 1280;
    int   windowedHeight   = 720;

    // --- HUD ---
    bool showFps        = false;
    bool showControlsHint = true;

    // --- Post-process ---
    float bloomIntensity = 1.0f;   // 0..2

    // --- Keybinds ---
    Keybinds keys;
};

// Process-wide singleton accessor. Safe to call from any thread on the main
// GLFW context; writes from the settings UI thread and reads from the game
// loop thread all go through this one Data instance.
Data& get();

// Load settings.json from %APPDATA%\NeonObby (missing file → defaults).
// Called once at startup.
void load();

// Flush current state to disk. Called when the user leaves the settings UI or
// at clean shutdown.
void save();

// Human-readable label for a GLFW key constant, e.g. GLFW_KEY_W → "W",
// GLFW_KEY_LEFT_CONTROL → "Left Ctrl". Used by SettingsUI to render the
// current binding in a button.
std::string keyName(int glfwKey);

} // namespace Settings
