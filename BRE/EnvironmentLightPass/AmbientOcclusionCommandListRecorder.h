#pragma once

#include <DirectXMath.h>
#include <vector>

#include <CommandManager\CommandListPerFrame.h>
#include <ResourceManager\FrameUploadCBufferPerFrame.h>
#include <ResourceManager\UploadBuffer.h>

namespace BRE {
struct FrameCBuffer;

///
/// @brief Responsible of command list recording for ambient occlusion pass.
///
class AmbientOcclusionCommandListRecorder {
public:
    AmbientOcclusionCommandListRecorder() = default;
    ~AmbientOcclusionCommandListRecorder() = default;
    AmbientOcclusionCommandListRecorder(const AmbientOcclusionCommandListRecorder&) = delete;
    const AmbientOcclusionCommandListRecorder& operator=(const AmbientOcclusionCommandListRecorder&) = delete;
    AmbientOcclusionCommandListRecorder(AmbientOcclusionCommandListRecorder&&) = default;
    AmbientOcclusionCommandListRecorder& operator=(AmbientOcclusionCommandListRecorder&&) = default;

    ///
    /// @brief Initializes the pipeline state object and root signature
    ///
    /// This method must be called at the beginning of the application, and once.
    ///
    ///
    static void InitSharedPSOAndRootSignature() noexcept;

    ///
    /// @brief Initializes the command list recorder.
    ///
    /// InitSharedPSOAndRootSignature() must be called first and once
    /// 
    /// @param normalSmoothnessBuffer Geometry buffer that contains normal and smoothness factor
    /// @param depthBuffer Depth buffer
    /// @param renderTargetView CPU descriptor handle of the render target
    ///
    void Init(ID3D12Resource& normalSmoothnessBuffer,
              ID3D12Resource& depthBuffer,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    ///
    /// @brief Records a command list and push it into the CommandListExecutor
    ///
    /// Init() must be called first
    ///
    /// @param frameCBuffer Constant buffer per frame, for current frame
    ///
    void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

    ///
    /// @brief Validates internal data. Used most with assertions.
    ///
    bool IsDataValid() const noexcept;

private:
    ///
    /// @brief Creates the sample kernel buffer
    /// @param sampleKernel List of 4D coordinate vectors for the sample kernel
    ///
    void CreateSampleKernelBuffer(const std::vector<DirectX::XMFLOAT4>& sampleKernel) noexcept;

    ///
    /// @brief Creates the noise texture used in ambient occlusion
    /// @param noiseVector List of 4D noise vectors
    /// @return The created noise texture
    ///
    ID3D12Resource* CreateAndGetNoiseTexture(const std::vector<DirectX::XMFLOAT4>& noiseVector) noexcept;

    ///
    /// @brief Initializes ambient occlusion shaders resource views
    /// @param normalSmothnessBuffer Geometry buffer that contains normals and smoothness factor
    /// @param depthBuffer Depth buffer
    /// @param noiseTexture Noise texture created with CreateAndGetNoiseTexture
    /// @param sampleKernelSize Size of the sample kernel
    /// @see CreateSampleKernelBuffer
    /// @see CreateAndGetNoiseTexture
    ///
    void InitShaderResourceViews(ID3D12Resource& normalSmoothnessBuffer,
                                 ID3D12Resource& depthBuffer,
                                 ID3D12Resource& noiseTexture,
                                 const std::uint32_t sampleKernelSize) noexcept;

    ///
    /// @brief Initialize ambient occlusion constant buffer
    ///
    void InitAmbientOcclusionCBuffer() noexcept;

    CommandListPerFrame mCommandListPerFrame;

    FrameUploadCBufferPerFrame mFrameUploadCBufferPerFrame;

    UploadBuffer* mSampleKernelUploadBuffer{ nullptr };

    D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };

    D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView{ 0UL };

    UploadBuffer* mAmbientOcclusionUploadCBuffer{ nullptr };
};
}