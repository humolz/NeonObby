#pragma once

#include <string>

namespace Paths {
    // Returns the directory containing the running executable (no trailing slash).
    // Cached after first call.
    const std::string& getExeDir();

    // Returns absolute path to a file inside the assets directory.
    // Example: getAssetPath("shaders/neon.vert")
    std::string getAssetPath(const std::string& relative);
}
