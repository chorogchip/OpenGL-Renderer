#include "scene_async_loader.h"

#include <chrono>
#include <exception>
#include <utility>

#include "model_loader.h"

namespace chr {

    bool SceneAsyncLoader::start(const std::string& path) {
        if (status_ == SceneLoadStatus::Loading) {
            return false;
        }

        path_ = path;
        error_message_.clear();
        result_.reset();
        status_ = SceneLoadStatus::Loading;
        file_load_progress_.store(0.0f, std::memory_order_relaxed);
        future_ = std::async(std::launch::async, [path, this]() {
            return load_scene(path.c_str(), &file_load_progress_);
        });

        return true;
    }

    void SceneAsyncLoader::poll() {
        if (status_ != SceneLoadStatus::Loading || !future_.valid()) {
            return;
        }

        if (future_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            return;
        }

        try {
            SceneRaw loaded_scene = future_.get();
            if (loaded_scene.meshes.empty()) {
                error_message_ = "Scene loaded without renderable meshes.";
                status_ = SceneLoadStatus::Failed;
                return;
            }

            result_ = std::move(loaded_scene);
            status_ = SceneLoadStatus::Ready;
        }
        catch (const std::exception& exception) {
            error_message_ = exception.what();
            status_ = SceneLoadStatus::Failed;
        }
        catch (...) {
            error_message_ = "Unknown scene loading error.";
            status_ = SceneLoadStatus::Failed;
        }
    }

    SceneLoadSnapshot SceneAsyncLoader::snapshot() const {
        return { status_, path_, error_message_, file_load_progress_.load(std::memory_order_relaxed) };
    }

    bool SceneAsyncLoader::has_result() const {
        return result_.has_value();
    }

    SceneRaw SceneAsyncLoader::take_result() {
        SceneRaw scene;
        if (result_.has_value()) {
            scene = std::move(*result_);
            result_.reset();
            status_ = SceneLoadStatus::Idle;
        }

        return scene;
    }

}
