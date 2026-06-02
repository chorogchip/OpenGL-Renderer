#pragma once

#include <atomic>
#include <vector>

#include "scene/scene_raw.h"

namespace chr {
    SceneRaw load_scene(const char* filename, std::atomic<float>* progress = nullptr);
}
