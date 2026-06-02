#pragma once

#include <atomic>
#include <filesystem>

#include "scene/scene_raw.h"

// glTF scene loading via Assimp
// Loads meshes, materials, textures, and scene hierarchy
// Tracks loading progress for UI feedback
namespace chr {

    bool is_gltf_scene_path(const std::filesystem::path& path);
    SceneRaw load_gltf_scene(const std::filesystem::path& path, std::atomic<float>* progress = nullptr);

}
