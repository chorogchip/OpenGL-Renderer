#pragma once

#include <future>
#include <optional>
#include <string>

#include "scene_raw.h"

namespace chr {

    enum class SceneLoadStatus {
        Idle,
        Loading,
        Ready,
        Failed
    };

    struct SceneLoadSnapshot {
        SceneLoadStatus status = SceneLoadStatus::Idle;
        std::string path;
        std::string error_message;
        float file_progress = 0.0f;
    };

    class SceneAsyncLoader {
    public:
        SceneAsyncLoader() = default;
        SceneAsyncLoader(const SceneAsyncLoader&) = delete;
        SceneAsyncLoader& operator=(const SceneAsyncLoader&) = delete;

        bool start(const std::string& path);
        void poll();

        [[nodiscard]] SceneLoadSnapshot snapshot() const;
        [[nodiscard]] bool has_result() const;
        [[nodiscard]] SceneRaw take_result();

    private:
        SceneLoadStatus status_ = SceneLoadStatus::Idle;
        std::string path_;
        std::string error_message_;
        std::future<SceneRaw> future_;
        std::optional<SceneRaw> result_;
        std::atomic<float> file_load_progress_{0.0f};
    };

}
