#pragma once

#include "renderer/Model.h"
#include <memory>
#include <string>

class ModelLoader {
public:
    // Load a .glb (glTF binary) file. Returns nullptr on failure.
    static std::unique_ptr<Model> load(const std::string& path);
};
