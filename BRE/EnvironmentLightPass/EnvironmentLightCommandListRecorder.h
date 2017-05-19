#pragma once

#include <wrl.h>

#include <CommandManager\CommandListPerFrame.h>
#include <ResourceManager\FrameUploadCBufferPerFrame.h>

namespace BRE {
struct FrameCBuffer;

///
/// @brief Responsible of recording of command lists for environment light pass.
///
class EnvironmentLightCommandListRecorder {
public:
    EnvironmentLightCommandListRecorder() = default;
    ~EnvironmentLightCommandListRecorder() = default;
    EnvironmentLightCommandListRecorder(const EnvironmentLightCommandListRecorder&) = delete;
    const EnvironmentLightCommandListRecorder& operator=(const EnvironmentLightCommandListRecorder&) = delete;
    EnvironmentLightCommandListRecorder(EnvironmentLightCommandListRecorder&&) = default;
    EnvironmentLightCommandListRecorder& operator=(EnvironmentLightCommandListRecorder&&) = default;

    ///
    /// @brief Initializes pipeline state object and root signature
    ///
    /// This method must be called at the beginning of the application, and once
    ///
    static void InitSharedPSOAndRootSignature() noexcept;

    ///
    /// @brief Initializes the recorder. 
    ///
    /// InitSharedPSOAndRootSignature() must be called first
    ///
    /// @param normalSmoothnessBuffer Geometry buffer that contains normals and smoothness factors.
    /// @param baseColorMetalMaskBuffer Geometry buffer that contains base color and metal mask.
    /// @param depthBuffer Depth buffer
    /// @param diffuseIrradianceCubeMap Diffuse irradiance environment cube map
    /// @param specularPreConvolvedCubeMap Specular pre convolved environment cube map
    /// @param ambientAccessibilityBuffer Ambient accessibility buffer
    /// @param renderTargetView Render target view
    ///
    void Init(ID3D12Resource& normalSmoothnessBuffer,
              ID3D12Resource& baseColorMetalMaskBuffer,
              ID3D12Resource& depthBuffer,
              ID3D12Resource& diffuseIrradianceCubeMap,
              ID3D12Resource& specularPreConvolvedCubeMap,
              ID3D12Resource& ambientAccessibilityBuffer,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    ///
    /// @brief Records command lists and pushes them into CommandListExecutor
    ///
    /// Init() must be called first
    ///
    /// @param frameCBuffer Constant buffer per frame, for current frame
    ///
    void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool ValidateData() const noexcept;

private:
    ///
    /// @brief Initializes shader resource views
    /// @param normalSmoothnessBuffer Geometry buffer that contains normals and smoothness factors.
    /// @param baseColorMetalMaskBuffer Geometry buffer that contains base color and metal mask.
    /// @param depthBuffer Depth buffer
    /// @param diffuseIrradianceCubeMap Diffuse irradiance environment cube map
    /// @param ambientAccessibilityBuffer Ambient accessibility buffer
    /// @param specularPreConvolvedCubeMap Specular pre convolved environment cube map
    ///
    void InitShaderResourceViews(ID3D12Resource& normalSmoothnessBuffer,
                                 ID3D12Resource& baseColorMetalMaskBuffer,
                                 ID3D12Resource& depthBuffer,
                                 ID3D12Resource& diffuseIrradianceCubeMap,
                                 ID3D12Resource& ambientAccessibilityBuffer,
                                 ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    FrameUploadCBufferPerFrame mFrameUploadCBufferPerFrame;

    D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };

    D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView{ 0UL };
};
}