#include "model_loader.h"

#include <filesystem>
#include <iostream>

#include "gltf_scene_loader.h"

namespace chr {

    SceneRaw load_scene(const char* filename, std::atomic<float>* progress) {
        const std::filesystem::path model_path(filename);
        if (is_gltf_scene_path(model_path)) {
            return load_gltf_scene(model_path, progress);
        }

        SceneRaw result;
        if (!model_path.empty()) {
            std::cerr << "Unsupported scene format for fastgltf loader: " << filename << std::endl;
        }
        return result;
    }

}
