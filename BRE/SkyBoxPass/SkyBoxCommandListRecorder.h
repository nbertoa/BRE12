#pragma once

#include <DirectXMath.h>

#include <CommandManager\CommandListPerFrame.h>
#include <MathUtils\MathUtils.h>
#include <ResourceManager\FrameUploadCBufferPerFrame.h>
#include <ResourceManager/VertexAndIndexBufferCreator.h>

namespace BRE {
struct FrameCBuffer;

///
/// @brief Responsible to generate command list recorders for the sky box pass
///
class SkyBoxCommandListRecorder {
public:
    SkyBoxCommandListRecorder() = default;
    ~SkyBoxCommandListRecorder() = default;
    SkyBoxCommandListRecorder(const SkyBoxCommandListRecorder&) = delete;
    const SkyBoxCommandListRecorder& operator=(const SkyBoxCommandListRecorder&) = delete;
    SkyBoxCommandListRecorder(SkyBoxCommandListRecorder&&) = delete;
    SkyBoxCommandListRecorder& operator=(SkyBoxCommandListRecorder&&) = delete;

    ///
    /// @brief Initializes pipeline state object and root signature
    ///
    /// This method must be called at the beginning of the application, and once
    ///
    static void InitSharedPSOAndRootSignature() noexcept;

    ///
    /// @brief Initializes the command list recorder
    ///
    /// InitSharedPSOAndRootSignature() must be called first
    ///
    /// @param vertexBufferData Vertex buffer data of the sky box 
    /// @param indexBufferData Index buffer data of the sky box
    /// @param worldMatrix World matrix of the sky box
    /// @param skyBoxCubeMap Sky box cube map
    /// @param renderTargetView Render target view
    /// @param depthBufferView Depth buffer view
    ///
    void Init(const VertexAndIndexBufferCreator::VertexBufferData& vertexBufferData,
              const VertexAndIndexBufferCreator::IndexBufferData indexBufferData,
              const DirectX::XMFLOAT4X4& worldMatrix,
              ID3D12Resource& skyBoxCubeMap,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView,
              const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept;

    ///
    /// @brief Records and pushes command lists to CommandListExecutor
    /// @param frameCBuffer Constant buffer per frame, for current frame
    ///
    void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

private:
    ///
    /// @brief Initializes constant buffers
    /// @param worldMatrix World matrix
    ///
    void InitConstantBuffers(const DirectX::XMFLOAT4X4& worldMatrix) noexcept;

    ///
    /// @brief Initializes shader resource views
    /// @param skyBoxCubeMap Sky box cube map resource
    ///
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

