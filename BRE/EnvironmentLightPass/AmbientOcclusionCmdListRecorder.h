#pragma once

#include <CommandManager\CommandListPerFrame.h>
#include <ResourceManager\FrameUploadCBufferPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

namespace BRE {
struct FrameCBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for ambient occlusion pass.
class AmbientOcclusionCmdListRecorder {
public:
    AmbientOcclusionCmdListRecorder() = default;
    ~AmbientOcclusionCmdListRecorder() = default;
    AmbientOcclusionCmdListRecorder(const AmbientOcclusionCmdListRecorder&) = delete;
    const AmbientOcclusionCmdListRecorder& operator=(const AmbientOcclusionCmdListRecorder&) = delete;
    AmbientOcclusionCmdListRecorder(AmbientOcclusionCmdListRecorder&&) = default;
    AmbientOcclusionCmdListRecorder& operator=(AmbientOcclusionCmdListRecorder&&) = default;

    static void InitSharedPSOAndRootSignature() noexcept;

    // Preconditions:
    // - InitSharedPSOAndRootSignature() must be called first and once
    void Init(ID3D12Resource& normalSmoothnessBuffer,
              ID3D12Resource& depthBuffer,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    // Preconditions:
    // - Init() must be called first
    void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

    bool ValidateData() const noexcept;

private:
    void CreateSampleKernelBuffer(const void* sampleKernel, const std::uint32_t sampleKernelSize) noexcept;
    ID3D12Resource* CreateAndGetNoiseTexture(const void* noiseVectors, const std::uint32_t noiseTextureDimension) noexcept;
    void InitShaderResourceViews(ID3D12Resource& normalSmoothnessBuffer,
                                 ID3D12Resource& depthBuffer,
                                 ID3D12Resource& noiseTexture,
                                 const std::uint32_t sampleKernelSize) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    FrameUploadCBufferPerFrame mFrameUploadCBufferPerFrame;

    UploadBuffer* mSampleKernelUploadBuffer{ nullptr };

    D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };

    D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView{ 0UL };
};
}

