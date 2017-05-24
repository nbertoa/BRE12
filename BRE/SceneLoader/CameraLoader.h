#pragma once

#include <Camera\Camera.h>

namespace YAML {
class Node;
}

namespace BRE {

///
/// @brief Responsible to load from scene file the camera configuration
///
class CameraLoader {
public:
    CameraLoader() = default;
    CameraLoader(const CameraLoader&) = delete;
    const CameraLoader& operator=(const CameraLoader&) = delete;
    CameraLoader(CameraLoader&&) = delete;
    CameraLoader& operator=(CameraLoader&&) = delete;

    ///
    /// @brief Load camera settings
    /// @param rootNode Scene YAML file root node
    ///
    void LoadCamera(const YAML::Node& rootNode) noexcept;

    ///
    /// @brief Get camera
    /// @return Camera
    ///
    const Camera& GetCamera() const noexcept
    {
        return mCamera;
    }

private:
    Camera mCamera;
};
}