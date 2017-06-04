#pragma once

#include <memory>

#include <CommandManager\CommandListPerFrame.h>
#include <ToneMappingPass\ToneMappingCommandListRecorder.h>

namespace BRE {
///
/// Responsible to generate command lists recorders for the post process tone mapping effect
///
class ToneMappingPass {
public:
    ToneMappingPass() = default;
    ~ToneMappingPass() = default;
    ToneMappingPass(const ToneMappingPass&) = delete;
    const ToneMappingPass& operator=(const ToneMappingPass&) = delete;
    ToneMappingPass(ToneMappingPass&&) = delete;
    ToneMappingPass& operator=(ToneMappingPass&&) = delete;

    ///
    /// @brief Initializes the tone mapping pass
    /// @param inputColorBuffer Input buffer to apply tone mapping
    /// @param outputColorBuffer Output buffer with the tone mapping applied
    /// @param renderTargetView CPU descriptor of the render target
    ///
    void Init(ID3D12Resource& inputColorBuffer,
              ID3D12Resource& outputColorBuffer,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    ///
    /// @brief Executes the pass
    ///
    /// Init() must be called first. This method can record and
    /// push command lists to the CommandListExecutor.
    ///
    /// @return The number of recorded command lists.
    ///
    std::uint32_t Execute() noexcept;

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

    CommandListPerFrame mPrePassCommandListPerFrame;

    ID3D12Resource* mInputColorBuffer{ nullptr };
    ID3D12Resource* mOutputColorBuffer{ nullptr };

    std::unique_ptr<ToneMappingCommandListRecorder> mCommandListRecorder;
};
}