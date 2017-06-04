#pragma once

#include <memory>

#include <CommandManager\CommandListPerFrame.h>
#include <EnvironmentLightPass\AmbientOcclusionCommandListRecorder.h>
#include <EnvironmentLightPass\BlurCommandListRecorder.h>
#include <EnvironmentLightPass\EnvironmentLightCommandListRecorder.h>

namespace BRE {
///
/// @brief Pass responsible to apply ambient lighting and ambient occlusion
/// 
class EnvironmentLightPass {
public:
    EnvironmentLightPass() = default;
    ~EnvironmentLightPass() = default;
    EnvironmentLightPass(const EnvironmentLightPass&) = delete;
    const EnvironmentLightPass& operator=(const EnvironmentLightPass&) = delete;
    EnvironmentLightPass(EnvironmentLightPass&&) = delete;
    EnvironmentLightPass& operator=(EnvironmentLightPass&&) = delete;

    ///
    /// @brief Initializes the pass
    /// @param baseColorMetalMaskBuffer Geometry buffer that contains base color and metal mask.
    /// @param normalSmoothnessBuffer Geometry buffer that contains normals and smoothness factors.
    /// @param depthBuffer Depth buffer
    /// @param diffuseIrradianceCubeMap Diffuse irradiance environment cube map
    /// @param depthBufferShaderResourceView Shader resource view to the depth buffer
    /// @param specularPreConvolvedCubeMap Specular pre convolved environment cube map
    /// @param outputColorBufferRenderTargetView Render target view to the output color buffer
    /// @param depthBufferShaderResourceView Shader resource view to the depth buffer
    ///
    void Init(ID3D12Resource& baseColorMetalMaskBuffer,
              ID3D12Resource& normalSmoothnessBuffer,
              ID3D12Resource& depthBuffer,
              ID3D12Resource& diffuseIrradianceCubeMap,
              ID3D12Resource& specularPreConvolvedCubeMap,              
              const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferRenderTargetView,
              const D3D12_GPU_DESCRIPTOR_HANDLE& depthBufferShaderResourceView) noexcept;

    ///
    /// @brief Executes the pass
    ///
    /// Init() must be called first. This method can record and
    /// push command lists to the CommandListExecutor.
    ///
    /// @param frameCBuffer Constant buffer per frame, for current frame
    /// @return The number of recorded command lists.
    ///
    std::uint32_t Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

    ///
    /// @brief Records pre pass command lists and pushes them to 
    /// the CommandListExecutor.
    /// @return The number of recorded command lists
    ///
    std::uint32_t RecordAndPushPrePassCommandLists() noexcept;

    ///
    /// @brief Records middle pass command lists and pushes them to 
    /// the CommandListExecutor.
    /// @return The number of recorded command lists
    ///
    std::uint32_t RecordAndPushMiddlePassCommandLists() noexcept;

    ///
    /// @brief Records post pass command lists and pushes them to 
    /// the CommandListExecutor.
    /// @return The number of recorded command lists
    ///
    std::uint32_t RecordAndPushPostPassCommandLists() noexcept;

    CommandListPerFrame mPrePassCommandListPerFrame;
    CommandListPerFrame mMiddlePassCommandListPerFrame;
    CommandListPerFrame mPostPassCommandListPerFrame;

    ID3D12Resource* mAmbientAccessibilityBuffer{ nullptr };
    D3D12_GPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferShaderResourceView{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferRenderTargetView{ 0UL };

    ID3D12Resource* mBlurBuffer{ nullptr };
    D3D12_GPU_DESCRIPTOR_HANDLE mBlurBufferShaderResourceView{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mBlurBufferRenderTargetView{ 0UL };

    std::unique_ptr<AmbientOcclusionCommandListRecorder> mAmbientOcclusionRecorder;
    std::unique_ptr<BlurCommandListRecorder> mBlurRecorder;
    std::unique_ptr<EnvironmentLightCommandListRecorder> mEnvironmentLightRecorder;

    ID3D12Resource* mBaseColorMetalMaskBuffer{ nullptr };
    ID3D12Resource* mNormalSmoothnessBuffer{ nullptr };
    ID3D12Resource* mDepthBuffer{ nullptr };
};
}