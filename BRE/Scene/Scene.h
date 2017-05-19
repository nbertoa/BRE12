#pragma once

#include <vector>

#include <Camera\Camera.h>
#include <GeometryPass/GeometryCommandListRecorder.h>

namespace BRE {
///
/// @brief Represents a scene (geometry pass command list recorders, lighting configurations, camera, etc)
///
class Scene {
public:
    Scene() = default;
    Scene(const Scene&) = delete;
    const Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    ///
    /// @brief Get geometry pass command list recorders
    /// @return Geometry pass command list recorders
    ///
    GeometryCommandListRecorders& GetGeometryCommandListRecorders() noexcept;

    ///
    /// @brief Get sky box cube map resource
    /// @return Sky box cube map resource
    ///
    ID3D12Resource* &GetSkyBoxCubeMap() noexcept;

    ///
    /// @brief Get diffuse irradiance environment cube map
    /// @return Diffuse irradiance environment cube map
    ///
    ID3D12Resource* &GetDiffuseIrradianceCubeMap() noexcept;

    ///
    /// @brief Get specular pre convolved environment cube map
    /// @return Specular pre convolved environment cube map
    ///
    ID3D12Resource* &GetSpecularPreConvolvedCubeMap() noexcept;

    Camera& GetCamera() noexcept
    {
        return mCamera;
    }

private:
    GeometryCommandListRecorders mGeometryCommandListRecorders;

    ID3D12Resource* mSkyBoxCubeMap{ nullptr };
    ID3D12Resource* mDiffuseIrradianceCubeMap{ nullptr };
    ID3D12Resource* mSpecularPreConvolvedCubeMap{ nullptr };

    Camera mCamera;
};
}