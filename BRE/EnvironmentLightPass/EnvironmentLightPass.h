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
    /// @param specularPreConvolvedCubeMap Specular pre convolved environment cube map
    ///
    void Init(ID3D12Resource& baseColorMetalMaskBuffer,
              ID3D12Resource& normalSmoothnessBuffer,
              ID3D12Resource& depthBuffer,
              ID3D12Resource& diffuseIrradianceCubeMap,
              ID3D12Resource& specularPreConvolvedCubeMap,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    ///
    /// @brief Executes the pass
    /// @param frameCBuffer Constant buffer per frame, for current frame
    ///
    void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

    ///
    /// @brief Executes begin task for the pass
    ///
    void ExecuteBeginTask() noexcept;

    ///
    /// @brief Executes middle task for the pass
    ///
    void ExecuteMiddleTask() noexcept;

    ///
    /// @brief Executes final task for the pass
    /// @return Returns true if the final task command list was pushed to 
    /// the CommandListExecutor. Otherwise, false.
    ///
    bool ExecuteFinalTask() noexcept;

    CommandListPerFrame mBeginCommandListPerFrame;
    CommandListPerFrame mMiddleCommandListPerFrame;
    CommandListPerFrame mFinalCommandListPerFrame;

    ID3D12Resource* mAmbientAccessibilityBuffer{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferRenderTargetView{ 0UL };

    ID3D12Resource* mBlurBuffer{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE mBlurBufferRenderTargetView{ 0UL };

    std::unique_ptr<AmbientOcclusionCommandListRecorder> mAmbientOcclusionRecorder;
    std::unique_ptr<BlurCommandListRecorder> mBlurRecorder;
    std::unique_ptr<EnvironmentLightCommandListRecorder> mEnvironmentLightRecorder;

    ID3D12Resource* mBaseColorMetalMaskBuffer{ nullptr };
    ID3D12Resource* mNormalSmoothnessBuffer{ nullptr };
    ID3D12Resource* mDepthBuffer{ nullptr };
};
}