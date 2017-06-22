#pragma once

#include <CommandManager\CommandListPerFrame.h>
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
    /// @param baseColorMetalnessBuffer Geometry buffer that contains base color and metalness.
    /// @param normalRoughnessBuffer Geometry buffer that contains normals and roughness factors.
    /// @param depthBuffer Depth buffer
    /// @param diffuseIrradianceCubeMap Diffuse irradiance environment cube map
    /// @param specularPreConvolvedCubeMap Specular pre convolved environment cube map
    /// @param ambientAccessibilityBuffer Ambient accessibility buffer
    /// @param outputColorBufferRenderTargetView Render target view to the output color buffer
    /// @param geometryBufferShaderResourceViewsBegin Shader resource view 
    /// to the first geometry buffer. The geometry buffer shader resource views are contiguous.
    /// @param ambientAccessibilityBufferShaderResourceView Shader resource view to the ambient accessibility buffer
    /// @param depthBufferShaderResourceView Shader resource view to the depth buffer
    ///
    void Init(ID3D12Resource& baseColorMetalnessBuffer,
              ID3D12Resource& normalRoughnessBuffer,
              ID3D12Resource& depthBuffer,
              ID3D12Resource& diffuseIrradianceCubeMap,
              ID3D12Resource& specularPreConvolvedCubeMap,
              ID3D12Resource& ambientAccessibilityBuffer,
              const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferRenderTargetView,
              const D3D12_GPU_DESCRIPTOR_HANDLE& geometryBufferShaderResourceViewsBegin,
              const D3D12_GPU_DESCRIPTOR_HANDLE& ambientAccessibilityBufferShaderResourceView,
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

    CommandListPerFrame mPrePassCommandListPerFrame;
    CommandListPerFrame mMiddlePassCommandListPerFrame;
    CommandListPerFrame mPostPassCommandListPerFrame;

    EnvironmentLightCommandListRecorder mEnvironmentLightRecorder;

    ID3D12Resource* mBaseColorMetalnessBuffer{ nullptr };
    ID3D12Resource* mNormalRoughnessBuffer{ nullptr };
    ID3D12Resource* mDepthBuffer{ nullptr };
    ID3D12Resource* mAmbientAccessibilityBuffer{ nullptr };
};
}