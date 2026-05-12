#pragma once

#include <string>

#include "scene_async_loader.h"

namespace chr {

    struct LoadingScreenState {
        SceneLoadStatus status = SceneLoadStatus::Idle;
        std::string title = "Loading Scene";
        std::string message;
        float progress = -1.0f;
    };

    void draw_loading_screen(const LoadingScreenState& state, int framebuffer_width, int framebuffer_height);
    LoadingScreenState make_loading_screen_state(const SceneLoadSnapshot& snapshot);

}
