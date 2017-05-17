#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

#include <DXUtils/D3DFactory.h>

namespace BRE {
///
/// @brief Responsible to create pipeline state objects
///
class PSOManager {
public:
    PSOManager() = delete;
    ~PSOManager() = delete;
    PSOManager(const PSOManager&) = delete;
    const PSOManager& operator=(const PSOManager&) = delete;
    PSOManager(PSOManager&&) = delete;
    PSOManager& operator=(PSOManager&&) = delete;

    ///
    /// @brief Releases all pipeline state objects
    ///
    static void Clear() noexcept;

    struct PSOCreationData {
        PSOCreationData() = default;
        ~PSOCreationData() = default;
        PSOCreationData(const PSOCreationData&) = delete;
        const PSOCreationData& operator=(const PSOCreationData&) = delete;
        PSOCreationData(PSOCreationData&&) = delete;
        PSOCreationData& operator=(PSOCreationData&&) = delete;

        ///
        /// @brief Checks if internal data is valid. Typically, used with assertions.
        /// @return True if valid. Otherwise, false.
        ///
        bool IsDataValid() const noexcept;

        std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayoutDescriptors{};

        ID3D12RootSignature* mRootSignature{ nullptr };

        // If a shader bytecode is not valid, then we do not load it.
        D3D12_SHADER_BYTECODE mVertexShaderBytecode{ 0UL };
        D3D12_SHADER_BYTECODE mGeometryShaderBytecode{ 0UL };
        D3D12_SHADER_BYTECODE mDomainShaderBytecode{ 0UL };
        D3D12_SHADER_BYTECODE mHullShaderBytecode{ 0UL };
        D3D12_SHADER_BYTECODE mPixelShaderBytecode{ 0UL };

        D3D12_BLEND_DESC mBlendDescriptor = D3DFactory::GetDisabledBlendDesc();
        D3D12_RASTERIZER_DESC mRasterizerDescriptor = D3DFactory::GetDefaultRasterizerDesc();
        D3D12_DEPTH_STENCIL_DESC mDepthStencilDescriptor = D3DFactory::GetDefaultDepthStencilDesc();
        std::uint32_t mNumRenderTargets{ 0U };
        DXGI_FORMAT mRenderTargetFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{ DXGI_FORMAT_UNKNOWN };
        DXGI_SAMPLE_DESC mSampleDescriptor{ 1U, 0U };
        std::uint32_t mSampleMask{ UINT_MAX };
        D3D12_PRIMITIVE_TOPOLOGY_TYPE mPrimitiveTopologyType{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
    };

    ///
    /// @brief Create graphics pipeline state object
    /// @param psoCreationData Pipeline state object creation data. It must be valid
    /// @return Pipeline state object
    ///
    static ID3D12PipelineState& CreateGraphicsPSO(const PSOManager::PSOCreationData& psoCreationData) noexcept;

private:
    ///
    /// @brief Create graphics pipeline state object by descriptor
    /// @param psoDescriptor Graphics pipeline state object descriptor
    /// @return Pipeline state object
    ///
    static ID3D12PipelineState& CreateGraphicsPSOByDescriptor(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDescriptor) noexcept;

    static tbb::concurrent_unordered_set<ID3D12PipelineState*> mPSOs;

    static std::mutex mMutex;
};
}