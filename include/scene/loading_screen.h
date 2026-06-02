#pragma once

#include <string>

#include "scene/scene_async_loader.h"

// Loading screen UI shown while scene is being loaded
// Displays progress bar and status messages
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
