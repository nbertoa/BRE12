#pragma once

#include <DirectXMath.h>

#include <CommandManager\CommandListPerFrame.h>
#include <MathUtils\MathUtils.h>
#include <ResourceManager\FrameUploadCBufferPerFrame.h>
#include <ResourceManager/VertexAndIndexBufferCreator.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12CommandAllocator;
struct ID3D12Resource;
struct ID3D12GraphicsCommandList;

namespace BRE {
struct FrameCBuffer;

class SkyBoxCmdListRecorder {
public:
    SkyBoxCmdListRecorder() = default;
    ~SkyBoxCmdListRecorder() = default;
    SkyBoxCmdListRecorder(const SkyBoxCmdListRecorder&) = delete;
    const SkyBoxCmdListRecorder& operator=(const SkyBoxCmdListRecorder&) = delete;
    SkyBoxCmdListRecorder(SkyBoxCmdListRecorder&&) = delete;
    SkyBoxCmdListRecorder& operator=(SkyBoxCmdListRecorder&&) = delete;

    static void InitSharedPSOAndRootSignature() noexcept;

    // Preconditions:
    // - InitSharedPSOAndRootSignature() must be called first and once
    void Init(const VertexAndIndexBufferCreator::VertexBufferData& vertexBufferData,
              const VertexAndIndexBufferCreator::IndexBufferData indexBufferData,
              const DirectX::XMFLOAT4X4& worldMatrix,
              ID3D12Resource& skyBoxCubeMap,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView,
              const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept;

    // Preconditions:
    // - Init() must be called first
    void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

    bool IsDataValid() const noexcept;

private:
    void InitConstantBuffers(const DirectX::XMFLOAT4X4& worldMatrix) noexcept;
    void InitShaderResourceViews(ID3D12Resource& skyBoxCubeMap) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    VertexAndIndexBufferCreator::VertexBufferData mVertexBufferData;
    VertexAndIndexBufferCreator::IndexBufferData mIndexBufferData;

    FrameUploadCBufferPerFrame mFrameUploadCBufferPerFrame;

    UploadBuffer* mObjectUploadCBuffer{ nullptr };
    D3D12_GPU_DESCRIPTOR_HANDLE mObjectCBufferView;

    D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView;

    D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferView{ 0UL };
};
}

