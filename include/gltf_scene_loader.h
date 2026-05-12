#pragma once

#include <filesystem>

#include "scene_raw.h"

namespace chr {

    bool is_gltf_scene_path(const std::filesystem::path& path);
    SceneRaw load_gltf_scene(const std::filesystem::path& path);

}
