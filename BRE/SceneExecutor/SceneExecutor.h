#pragma once

namespace BRE {
class RenderManager;
class Scene;

///
/// @brief Responsible to load and execute the scene
///
class SceneExecutor {
public:
    ///
    /// @brief SceneExecutor constructor
    /// @param sceneFilePath Scene file path. Must be not nullptr.
    ///
    explicit SceneExecutor(const char* sceneFilePath);
    ~SceneExecutor();
    SceneExecutor(const SceneExecutor&) = delete;
    const SceneExecutor& operator=(const SceneExecutor&) = delete;
    SceneExecutor(SceneExecutor&&) = delete;
    SceneExecutor& operator=(SceneExecutor&&) = delete;

    ///
    /// @brief Executes the scene executor.
    ///
    /// This method is going to load the scene and run the main loop.
    ///
    void Execute() noexcept;

private:
    Scene* mScene{ nullptr };

    RenderManager* mRenderManager{ nullptr };
};
}