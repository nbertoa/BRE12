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
    enum Buffers {
        NORMAL_SMOOTHNESS = 0U, // 2 encoded normals based on octahedron encoding + 1 smoothness
        BASECOLOR_METALMASK, // 3 base color + 1 metal mask
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
    /// @param depthBufferView Dpeth buffer view
    ///
    void Init(const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept;

    ///
    /// @brief Get geometry buffers
    /// @return List of geometry buffers
    ///
    __forceinline ID3D12Resource* *GetGeometryBuffers() noexcept
    {
        return mGeometryBuffers;
    }

    ///
    /// @brief Executes the geometry pass
    ///
    /// Init() must be called first
    ///
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
    /// @brief Executes begin task for geometry pass
    ///
    void ExecuteBeginTask() noexcept;

    CommandListPerFrame mCommandListPerFrame;

    // Geometry buffers data
    ID3D12Resource* mGeometryBuffers[BUFFERS_COUNT]{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE mGeometryBufferRenderTargetViews[BUFFERS_COUNT];

    GeometryCommandListRecorders& mGeometryCommandListRecorders;
};
}