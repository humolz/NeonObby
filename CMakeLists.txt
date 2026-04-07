cmake_minimum_required(VERSION 3.20)
project(NeonObby VERSION 1.0.1 LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(NEONOBBY_SERVER_ONLY "Build only the headless server" OFF)
option(NEONOBBY_BUILD_TESTS "Build unit tests" ON)

# Compiler warnings
include(cmake/CompilerWarnings.cmake)

# Fetch dependencies
include(cmake/FetchDependencies.cmake)

# Generate Version.h from the project() version above so the auto-updater
# always knows which release it is.
configure_file(
    ${CMAKE_SOURCE_DIR}/src/Version.h.in
    ${CMAKE_BINARY_DIR}/generated/Version.h
    @ONLY
)

if(NOT NEONOBBY_SERVER_ONLY)
    # Optional Windows resource (icon + version info). Only included if the
    # .ico file exists, so the build still works before the user adds artwork.
    set(NEONOBBY_RC_FILE "")
    if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/resources/icon.ico")
        set(NEONOBBY_RC_FILE "${CMAKE_SOURCE_DIR}/resources/app.rc")
    endif()

    # Client executable
    add_executable(NeonObby
        src/main.cpp
        src/core/Window.cpp
        src/core/Paths.cpp
        src/renderer/Shader.cpp
        src/renderer/Mesh.cpp
        src/renderer/MeshFactory.cpp
        src/renderer/Camera.cpp
        src/renderer/Framebuffer.cpp
        src/renderer/PostProcess.cpp
        src/renderer/ModelLoader.cpp
        src/audio/AudioEngine.cpp
        src/net/Updater.cpp
        ${NEONOBBY_RC_FILE}
    )
    target_include_directories(NeonObby PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
        ${CMAKE_SOURCE_DIR}/extern/miniaudio
        ${CMAKE_SOURCE_DIR}/extern/cgltf
    )
    target_link_libraries(NeonObby PRIVATE glfw glad glm::glm nlohmann_json::nlohmann_json imgui)

    # Platform-specific OpenGL
    find_package(OpenGL REQUIRED)
    target_link_libraries(NeonObby PRIVATE OpenGL::GL)

    # Auto-updater needs WinHTTP for HTTPS calls to the GitHub Releases API.
    if(WIN32)
        target_link_libraries(NeonObby PRIVATE winhttp)
        # -mwindows switches subsystem CONSOLE → WINDOWS so no black box pops up.
        # Applied to *all* build types — players should never see a terminal
        # flash by, even during development, and Task Manager won't list a
        # conhost.exe child process any more.
        target_link_options(NeonObby PRIVATE -mwindows)
    endif()

    # Release-only: optimize, strip symbols.
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(NeonObby PRIVATE -O2)
        target_compile_definitions(NeonObby PRIVATE NDEBUG)
        if(CMAKE_STRIP)
            add_custom_command(TARGET NeonObby POST_BUILD
                COMMAND ${CMAKE_STRIP} $<TARGET_FILE:NeonObby>
                COMMENT "Stripping release binary"
            )
        endif()
    endif()

    # Install rules — used by `cmake --install build --prefix install/` to
    # produce the staging layout the Inno Setup script packages.
    install(TARGETS NeonObby
        RUNTIME DESTINATION .
    )
    install(DIRECTORY assets/
        DESTINATION assets
        PATTERN "blender"   EXCLUDE   # source .blend dir — never ship
        PATTERN "*.blend"   EXCLUDE
        PATTERN "*.blend1"  EXCLUDE
        PATTERN ".DS_Store" EXCLUDE
    )
endif()
