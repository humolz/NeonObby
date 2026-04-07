#pragma once

// In-game auto-updater. Polls the GitHub Releases API on a background thread,
// downloads the new installer if available, then runs it silently to upgrade
// the current install. UI lives in src/ui/UpdateNotification.h — this header
// is the data + control surface only.

#include <string>

namespace Updater {

enum class State {
    Idle,            // No check has been started yet
    Checking,        // HTTPS request to GitHub in flight
    UpToDate,        // Latest release matches our version (or older)
    UpdateAvailable, // Newer release found, waiting for user to click Download
    Downloading,     // Installer .exe download in progress
    ReadyToInstall,  // Installer downloaded successfully, waiting on Restart
    Failed           // Network error, parse error, or download failure
};

struct Status {
    State state = State::Idle;
    std::string latestVersion;   // e.g. "1.1.0" — only valid when state >= UpdateAvailable
    std::string downloadUrl;     // .exe asset URL from the release
    std::string error;           // Human-readable when state == Failed
    float downloadProgress = 0.0f; // 0.0..1.0, only meaningful in Downloading
};

// Kick off a non-blocking version check. Safe to call once at startup.
// State transitions: Idle → Checking → (UpToDate | UpdateAvailable | Failed).
void checkForUpdate();

// Begin downloading the installer to %TEMP%. No-op unless state == UpdateAvailable.
// State transitions: UpdateAvailable → Downloading → (ReadyToInstall | Failed).
void downloadUpdate();

// Launch the downloaded installer in silent mode and exit the game so it can
// replace files. No-op unless state == ReadyToInstall. Returns true if launch
// succeeded — caller should then close the game.
bool applyUpdate();

// Snapshot of current state. Cheap, thread-safe, suitable for per-frame UI.
Status getStatus();

// Block until any background thread finishes. Call before main() returns so
// no thread outlives static destruction.
void shutdown();

} // namespace Updater
