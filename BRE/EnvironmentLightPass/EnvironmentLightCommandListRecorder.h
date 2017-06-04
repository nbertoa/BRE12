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
    /// @param diffuseIrradianceCubeMap Diffuse irradiance environment cube map
    /// @param specularPreConvolvedCubeMap Specular pre convolved environment cube map
    /// @param outputColorBufferRenderTargetView Render target view to the output color buffer
    /// @param geometryBufferShaderResourceViewsBegin Shader resource view 
    /// to the first geometry buffer. The geometry buffer shader resource views are contiguous.
    /// @param ambientAccessibilityBufferShaderResourceView Shader resource view to the ambient accessibility buffer
    /// @param depthBufferShaderResourceView Depth buffer shader resource view
    ///
    void Init(ID3D12Resource& diffuseIrradianceCubeMap,
              ID3D12Resource& specularPreConvolvedCubeMap,
              const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferRenderTargetView,
              const D3D12_GPU_DESCRIPTOR_HANDLE& geometryBufferShaderResourceViewsBegin,
              const D3D12_GPU_DESCRIPTOR_HANDLE& ambientAccessibilityBufferShaderResourceView,
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
    /// @param diffuseIrradianceCubeMap Diffuse irradiance environment cube map
    /// @param specularPreConvolvedCubeMap Specular pre convolved environment cube map
    /// @param ambientAccessibilityBuffer Ambient accessibility buffer
    ///
    void InitShaderResourceViews(ID3D12Resource& diffuseIrradianceCubeMap,
                                 ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    FrameUploadCBufferPerFrame mFrameUploadCBufferPerFrame;

    D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferRenderTargetView{ 0UL };

    // First descriptor in the list. All the others are contiguous
    D3D12_GPU_DESCRIPTOR_HANDLE mGeometryBufferShaderResourceViewsBegin{ 0UL };

    D3D12_GPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferShaderResourceView{ 0UL };
    D3D12_GPU_DESCRIPTOR_HANDLE mDepthBufferShaderResourceView{ 0UL };

    // First descriptor in the list. All the others are contiguous
    D3D12_GPU_DESCRIPTOR_HANDLE mDiffuseAndSpecularIrradianceTextureShaderResourceViews{ 0UL };
};
}