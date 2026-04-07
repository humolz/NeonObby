# NeonObby

A neon-themed 3D obstacle course game built from scratch in C++20 and OpenGL.

[![Latest release](https://img.shields.io/github/v/release/humolz/NeonObby)](https://github.com/humolz/NeonObby/releases/latest)
[![Platform](https://img.shields.io/badge/platform-Windows-blue)](https://github.com/humolz/NeonObby/releases/latest)

## Download

Grab the latest installer from the [releases page](https://github.com/humolz/NeonObby/releases/latest):

```
NeonObbySetup-X.Y.Z.exe
```

Run it. The installer drops the game in `Program Files\NeonObby`, creates a Start Menu shortcut and an optional desktop shortcut, and registers an entry in **Add/Remove Programs**. Save data lives in `%APPDATA%\NeonObby\` and is preserved across uninstalls and updates.

> Windows may show a SmartScreen warning on first launch — click "More info" → "Run anyway". The app is safe; this is normal for unsigned indie releases.

The game has a built-in auto-updater that checks GitHub Releases on launch. New versions are downloaded silently in the background and applied the next time you exit.

## Controls

| Action | Default key |
|---|---|
| Move | W A S D |
| Jump | Space |
| Crouch | Left Ctrl |
| Prone | Z |
| Pause | Esc |
| Restart level | R |
| Toggle debug overlay | F3 |
| Free cursor | Tab |

Every binding can be remapped from **Settings → Controls** in the pause menu or main menu.

## Features

- **Three hand-built levels** of escalating difficulty (First Steps, Neon Gauntlet, Neon Nightmare)
- **Custom physics** with capsule collision, swept-collision response, slope handling
- **ECS architecture** — every gameplay system is data-driven and independently testable
- **Skinned character animation** loaded from glTF, with Walk / Jump / Crouch / Prone / Slide blending
- **Bloom + post-processing pipeline** built around the neon aesthetic
- **Spatial audio** via miniaudio with procedurally generated SFX
- **Per-level checkpoints** with auto-save and best-time tracking
- **Full settings menu** — display mode, V-Sync, FPS cap (60/120/144/240/360/Uncapped), FOV, mouse sens, master volume, full keybind remapping
- **Auto-updater** that pulls from the GitHub Releases API

## Building from source

### Prerequisites
- CMake 3.20+
- A C++20 compiler (MinGW Clang or MSVC on Windows)
- [Inno Setup 6](https://jrsoftware.org/isdl.php) — only needed if you want to package the installer

All other dependencies (GLFW, GLAD, GLM, ImGui, nlohmann/json, miniaudio, cgltf) are pulled automatically by CMake's `FetchContent`.

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
cmake --build build
```

The executable lands at `build/NeonObby.exe`. Asset paths are resolved at runtime relative to the binary, so the dev build works in place — no install step needed.

### Package the installer

```bash
cmake --install build --prefix install/
"C:\Users\<you>\AppData\Local\Programs\Inno Setup 6\ISCC.exe" installer/NeonObby.iss
```

Output: `dist/NeonObbySetup-X.Y.Z.exe`.

## Tech stack

| Layer | What |
|---|---|
| Language | C++20 |
| Graphics | OpenGL 4.1 + GLAD + custom forward renderer |
| Windowing / input | GLFW |
| UI | Dear ImGui |
| Physics | Custom capsule-vs-AABB sweep solver |
| Audio | miniaudio (single-header) |
| Asset loading | cgltf for skinned meshes, nlohmann/json for levels & saves |
| Build | CMake + FetchContent |
| Installer | Inno Setup 6 |
| Auto-updater | WinHTTP + GitHub Releases REST API |

## Project layout

```
src/
  audio/        AudioEngine + procedural SFX synthesis
  components/   ECS components (Transform, RigidBody, Collider, ...)
  core/         Window, Input, Settings, Paths, Timer
  ecs/          World + sparse-set entity store
  level/        LevelLoader (JSON → entity spawn)
  net/          Updater (GitHub Releases polling + silent install)
  physics/      PhysicsWorld, swept collision, gravity & friction
  player/       ThirdPersonCamera, PlayerController
  renderer/     Shader, Mesh, Camera, PostProcess, ParticleSystem, Model
  save/         SaveManager (%APPDATA% persistence)
  scene/        Scene base, MenuScene, GameScene, SceneManager
  systems/      ECS systems (PlayerControllerSystem, ObstacleSystem, ...)
  ui/           HUD, PauseMenu, MainMenuUI, SettingsUI, LevelCompleteUI

assets/
  audio/        (procedurally generated at runtime)
  fonts/
  levels/       level_01.json, level_02.json, level_03.json
  models/       pixeldude.glb (skinned player)
  shaders/      neon, skybox, particle, debug, skinned, bloom, postprocess

installer/      NeonObby.iss (Inno Setup script)
resources/      icon.ico, app.rc (Win32 resources)
```

## License

All rights reserved.
