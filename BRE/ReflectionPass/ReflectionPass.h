#pragma once

#include <CommandManager\CommandListPerFrame.h>

namespace BRE {
///
/// @brief Pass responsible to apply hi-Z screen space cone-traced reflections
/// 
class ReflectionPass {
public:
    ReflectionPass() = default;
    ~ReflectionPass() = default;
    ReflectionPass(const ReflectionPass&) = delete;
    const ReflectionPass& operator=(const ReflectionPass&) = delete;
    ReflectionPass(ReflectionPass&&) = delete;
    ReflectionPass& operator=(ReflectionPass&&) = delete;

    ///
    /// @brief Initializes the pass
    /// @param depthBuffer Depth buffer
    ///
    void Init(ID3D12Resource& depthBuffer) noexcept;

    ///
    /// @brief Executes the pass
    ///
    void Execute() noexcept;

private:
    ///
    /// @brief Initializes hierarchy z buffer
    ///
    void InitHierZBuffer() noexcept;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

    ///
    /// @brief Executes begin task for the pass
    ///
    void ExecuteBeginTask() noexcept;

    CommandListPerFrame mBeginCommandListPerFrame;

    ID3D12Resource* mHierZBuffer{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE mHierZBufferMipLevelRenderTargetViews[10U]{ 0UL };
    D3D12_GPU_DESCRIPTOR_HANDLE mHierZBufferMipLevelShaderResourceViews[10U]{ 0UL };

    ID3D12Resource* mDepthBuffer{ nullptr };
    D3D12_GPU_DESCRIPTOR_HANDLE mDepthBufferShaderResourceView{ 0UL };
};
}