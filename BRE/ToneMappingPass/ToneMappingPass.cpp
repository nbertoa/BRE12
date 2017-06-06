#include "ToneMappingPass.h"

#include <d3d12.h>
#include <DirectXColors.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DXUtils/d3dx12.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

namespace BRE {
void
ToneMappingPass::Init(ID3D12Resource& inputColorBuffer,
                      const D3D12_GPU_DESCRIPTOR_HANDLE& inputColorBufferShaderResourceView,
                      ID3D12Resource& outputColorBuffer,
                      const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferRenderTargetView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    ToneMappingCommandListRecorder::InitSharedPSOAndRootSignature();

    mInputColorBuffer = &inputColorBuffer;
    mOutputColorBuffer = &outputColorBuffer;    

    mCommandListRecorder.Init(inputColorBufferShaderResourceView,
                              outputColorBufferRenderTargetView);

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
ToneMappingPass::Execute() noexcept
{
    BRE_ASSERT(IsDataValid());

    std::uint32_t commandListCount = 0U;

    commandListCount += RecordAndPushPrePassCommandLists();
    commandListCount += mCommandListRecorder.RecordAndPushCommandLists();

    return commandListCount;
}

bool
ToneMappingPass::IsDataValid() const noexcept
{
    const bool b =
        mInputColorBuffer != nullptr &&
        mOutputColorBuffer != nullptr;

    return b;
}

std::uint32_t
ToneMappingPass::RecordAndPushPrePassCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());

    D3D12_RESOURCE_BARRIER barriers[2U];
    std::uint32_t barrierCount = 0UL;
    if (ResourceStateManager::GetResourceState(*mInputColorBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mInputColorBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(*mOutputColorBuffer) != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mOutputColorBuffer,
                                                                                        D3D12_RESOURCE_STATE_RENDER_TARGET);
        ++barrierCount;
    }

    if (barrierCount > 0UL) {
        ID3D12GraphicsCommandList& commandList = mPrePassCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);
        commandList.ResourceBarrier(barrierCount, barriers);
        BRE_CHECK_HR(commandList.Close());
        CommandListExecutor::Get().PushCommandList(commandList);

        return 1U;
    }

    return 0U;
}
}