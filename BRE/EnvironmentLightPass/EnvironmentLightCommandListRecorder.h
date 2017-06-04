#pragma once

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
    /// @param diffuseIrradianceCubeMap Diffuse irradiance environment cube map
    /// @param specularPreConvolvedCubeMap Specular pre convolved environment cube map
    /// @param ambientAccessibilityBuffer Ambient accessibility buffer
    /// @param outputColorBufferRenderTargetView Render target view to the output color buffer
    /// @param depthBufferShaderResourceView Depth buffer shader resource view
    ///
    void Init(ID3D12Resource& normalSmoothnessBuffer,
              ID3D12Resource& baseColorMetalMaskBuffer,
              ID3D12Resource& diffuseIrradianceCubeMap,
              ID3D12Resource& specularPreConvolvedCubeMap,
              ID3D12Resource& ambientAccessibilityBuffer,
              const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferRenderTargetView,
              const D3D12_GPU_DESCRIPTOR_HANDLE& depthBufferShaderResourceView) noexcept;

    ///
    /// @brief Records command lists and pushes them into CommandListExecutor
    ///
    /// Init() must be called first
    ///
    /// @param frameCBuffer Constant buffer per frame, for current frame
    /// @return The number of pushed command lists
    ///
    std::uint32_t RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

private:
    ///
    /// @brief Initializes shader resource views
    /// @param normalSmoothnessBuffer Geometry buffer that contains normals and smoothness factors.
    /// @param baseColorMetalMaskBuffer Geometry buffer that contains base color and metal mask.
    /// @param diffuseIrradianceCubeMap Diffuse irradiance environment cube map
    /// @param specularPreConvolvedCubeMap Specular pre convolved environment cube map
    /// @param ambientAccessibilityBuffer Ambient accessibility buffer
    ///
    void InitShaderResourceViews(ID3D12Resource& normalSmoothnessBuffer,
                                 ID3D12Resource& baseColorMetalMaskBuffer,
                                 ID3D12Resource& diffuseIrradianceCubeMap,
                                 ID3D12Resource& specularPreConvolvedCubeMap,
                                 ID3D12Resource& ambientAccessibilityBuffer) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    FrameUploadCBufferPerFrame mFrameUploadCBufferPerFrame;

    D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferRenderTargetView{ 0UL };
    D3D12_GPU_DESCRIPTOR_HANDLE mDepthBufferShaderResourceView{ 0UL };

    // First descriptor in the list. All the others are contiguous
    D3D12_GPU_DESCRIPTOR_HANDLE mPixelShaderResourceViewsBegin{ 0UL };
};
}