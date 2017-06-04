#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>

#include <CommandManager\CommandListPerFrame.h>
#include <ResourceManager\FrameUploadCBufferPerFrame.h>
#include <ResourceManager/VertexAndIndexBufferCreator.h>

namespace BRE {
struct FrameCBuffer;

///
/// @brief Responsible to record command lists for deferred shading geometry pass
///
/// Steps:
/// - Inherit from it and reimplement RecordAndPushCommandLists() method
/// - Call RecordAndPushCommandLists() to create command lists to execute in the GPU
///
class GeometryCommandListRecorder {
public:
    struct GeometryData {
        GeometryData() = default;

        VertexAndIndexBufferCreator::VertexBufferData mVertexBufferData;
        VertexAndIndexBufferCreator::IndexBufferData mIndexBufferData;
        std::vector<DirectX::XMFLOAT4X4> mWorldMatrices;
        std::vector<DirectX::XMFLOAT4X4> mInverseTransposeWorldMatrices;
    };

    GeometryCommandListRecorder() = default;
    virtual ~GeometryCommandListRecorder()
    {}

    GeometryCommandListRecorder(const GeometryCommandListRecorder&) = delete;
    const GeometryCommandListRecorder& operator=(const GeometryCommandListRecorder&) = delete;
    GeometryCommandListRecorder(GeometryCommandListRecorder&&) = default;
    GeometryCommandListRecorder& operator=(GeometryCommandListRecorder&&) = default;

    ///
    /// @brief Initializes the command list recorder
    /// @param geometryBufferRenderTargetViews Geometry buffers render target views. Must not be nullptr
    /// @param geometryBufferRenderTargetViewCount Geometry buffer render target views count. Must be greater than zero
    /// @param depthBufferView Depth buffer view
    ///
    void Init(const D3D12_CPU_DESCRIPTOR_HANDLE* geometryBufferRenderTargetViews,
              const std::uint32_t geometryBufferRenderTargetViewCount,
              const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept;

    ///
    /// @brief Records and pushes command lists to CommandListExecutor
    ///
    /// Init() must be called first
    ///
    /// @param frameCBuffer Constant buffer per frame, for current frame
    /// @return The number of pushed command lists
    ///
    virtual std::uint32_t RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept = 0;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
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

using GeometryCommandListRecorders = std::vector<std::unique_ptr<GeometryCommandListRecorder>>;
}