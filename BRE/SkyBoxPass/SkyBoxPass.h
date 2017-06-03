#pragma once

#include <memory>

#include <CommandManager\CommandListPerFrame.h>
#include <SkyBoxPass\SkyBoxCommandListRecorder.h>

namespace BRE {
struct FrameCBuffer;

///
/// @brief Responsible of execute command lists to generate a sky box
///
class SkyBoxPass {
public:
    SkyBoxPass() = default;
    ~SkyBoxPass() = default;
    SkyBoxPass(const SkyBoxPass&) = delete;
    const SkyBoxPass& operator=(const SkyBoxPass&) = delete;
    SkyBoxPass(SkyBoxPass&&) = delete;
    SkyBoxPass& operator=(SkyBoxPass&&) = delete;

    ///
    /// @brief Initializes the sky box pass
    /// @param skyBoxCubeMap Sky box cube map resource
    /// @param depthBuffer Depth buffer resource
    /// @param renderTargetView Render target view
    /// @param Depth buffer view
    ///
    void Init(ID3D12Resource& skyBoxCubeMap,
              ID3D12Resource& depthBuffer,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView,
              const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept;

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
    /// @brief Records pre pass command list
    /// @return True if a command list was recorded. Otherwise, false.
    ///
    bool RecordPrePassCommandList() noexcept;

    std::unique_ptr<SkyBoxCommandListRecorder> mCommandListRecorder;

    ID3D12Resource* mDepthBuffer{ nullptr };

    CommandListPerFrame mPrePassCommandListPerFrame;
};
}