#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>

#include <CommandManager\CommandListPerFrame.h>
#include <DXUtils/D3DFactory.h>
#include <ResourceManager\FrameUploadCBufferPerFrame.h>
#include <ResourceManager/VertexAndIndexBufferCreator.h>

namespace BRE {
struct FrameCBuffer;

// To record command lists for deferred shading geometry pass.
// Steps:
// - Inherit from it and reimplement RecordAndPushCommandLists() method
// - Call RecordAndPushCommandLists() to create command lists to execute in the GPU
class GeometryPassCmdListRecorder {
public:
    struct GeometryData {
        GeometryData() = default;

        VertexAndIndexBufferCreator::VertexBufferData mVertexBufferData;
        VertexAndIndexBufferCreator::IndexBufferData mIndexBufferData;
        std::vector<DirectX::XMFLOAT4X4> mWorldMatrices;
        std::vector<DirectX::XMFLOAT4X4> mInverseTransposeWorldMatrices;
    };
    using GeometryDataVector = std::vector<GeometryData>;

    GeometryPassCmdListRecorder() = default;
    virtual ~GeometryPassCmdListRecorder()
    {}

    GeometryPassCmdListRecorder(const GeometryPassCmdListRecorder&) = delete;
    const GeometryPassCmdListRecorder& operator=(const GeometryPassCmdListRecorder&) = delete;
    GeometryPassCmdListRecorder(GeometryPassCmdListRecorder&&) = default;
    GeometryPassCmdListRecorder& operator=(GeometryPassCmdListRecorder&&) = default;

    // Preconditions:
    // - "geometryBufferRenderTargetViews" must not be nullptr
    // - "geometryBufferRenderTargetViewCount" must be greater than zero
    void Init(const D3D12_CPU_DESCRIPTOR_HANDLE* geometryBufferRenderTargetViews,
              const std::uint32_t geometryBufferRenderTargetViewCount,
              const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept;

    // Preconditions:
    // - Init() must be called before
    virtual void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept = 0;

    // This method validates all data (nullptr's, etc)
    // When you inherit from this class, you should reimplement it to include
    // new members
    virtual bool IsDataValid() const noexcept;

protected:
    CommandListPerFrame mCommandListPerFrame;

    // Base command data. Once you inherits from this class, you should add
    // more class members that represent the extra information you need (like resources, for example)

    std::vector<GeometryData> mGeometryDataVec;

    FrameUploadCBufferPerFrame mFrameUploadCBufferPerFrame;

    UploadBuffer* mObjectUploadCBuffers{ nullptr };
    D3D12_GPU_DESCRIPTOR_HANDLE mStartObjectCBufferView;

    D3D12_GPU_DESCRIPTOR_HANDLE mStartMaterialCBufferView;
    UploadBuffer* mMaterialUploadCBuffers{ nullptr };

    const D3D12_CPU_DESCRIPTOR_HANDLE* mGeometryBufferRenderTargetViews{ nullptr };
    std::uint32_t mGeometryBufferRenderTargetViewCount{ 0U };

    D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferView{ 0UL };
};

using GeometryPassCommandListRecorders = std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>;
}

