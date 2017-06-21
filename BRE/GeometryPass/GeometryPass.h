#pragma once

#include <memory>
#include <vector>

#include <CommandManager\CommandListPerFrame.h>
#include <GeometryPass\GeometryCommandListRecorder.h>

namespace BRE {
struct FrameCBuffer;

///
/// @brief Responsible to execute command list recorders related with deferred shading geometry pass
///
class GeometryPass {
public:
    // Geometry buffers
    enum BufferType {
        NORMAL_ROUGHNESS = 0U, // 2 encoded normals based on octahedron encoding + 1 roughness
        BASECOLOR_METALNESS, // 3 base color + 1 metalness
        BUFFERS_COUNT
    };

    GeometryPass(GeometryCommandListRecorders& geometryPassCommandListRecorders);
    ~GeometryPass() = default;
    GeometryPass(const GeometryPass&) = delete;
    const GeometryPass& operator=(const GeometryPass&) = delete;
    GeometryPass(GeometryPass&&) = delete;
    GeometryPass& operator=(GeometryPass&&) = delete;

    ///
    /// @brief Initializes geometry pass
    /// @param depthBufferView Depth buffer view
    ///
    void Init(const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept;

    ///
    /// @brief Get geometry buffer by type
    /// @param bufferType Buffer type
    /// @return Geometry buffer
    ///
    __forceinline ID3D12Resource& GetGeometryBuffer(const BufferType bufferType) noexcept
    {
        return *mGeometryBuffers[bufferType];
    }

    ///
    /// @brief Get the shader resource view to the beginning of the contiguos list
    /// of shader resource views of the geometry buffers.
    /// @return The shader resource view to the beginning of the contiguos list
    /// where all the shader resource views for all the geometry buffers are stored.
    ///
    __forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetGeometryBufferShaderResourceViews() noexcept
    {
        return mGeometryBufferShaderResourceViews[0U];
    }

    ///
    /// @brief Get a shader resource view to a specific geometry buffer
    /// @return Shader resource view
    ///
    __forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetGeometryBufferShaderResourceView(const BufferType bufferType) noexcept
    {
        return mGeometryBufferShaderResourceViews[bufferType];
    }

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
    /// @brief Initializes shader resource views
    ///
    void InitShaderResourceViews() noexcept;

    CommandListPerFrame mPrePassCommandListPerFrame;

    // Geometry buffers data
    ID3D12Resource* mGeometryBuffers[BUFFERS_COUNT]{ nullptr };
    D3D12_GPU_DESCRIPTOR_HANDLE mGeometryBufferShaderResourceViews[BUFFERS_COUNT]{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mGeometryBufferRenderTargetViews[BUFFERS_COUNT]{ 0UL };

    GeometryCommandListRecorders& mGeometryCommandListRecorders;
};
}