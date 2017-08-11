#include "PostProcessPass.h"

#include <d3d12.h>
#include <DirectXColors.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DXUtils/d3dx12.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <Utils\DebugUtils.h>

using namespace DirectX;

namespace BRE {
void
PostProcessPass::Init(ID3D12Resource& inputColorBuffer,
                      const D3D12_GPU_DESCRIPTOR_HANDLE& inputColorBufferShaderResourceView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    mInputColorBuffer = &inputColorBuffer;

    PostProcessCommandListRecorder::InitSharedPSOAndRootSignature();

    mCommandListRecorder.Init(inputColorBufferShaderResourceView);

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
PostProcessPass::Execute(ID3D12Resource& frameBuffer,
                         const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferRenderTargetView) noexcept
{
    BRE_ASSERT(IsDataValid());
    BRE_ASSERT(frameBufferRenderTargetView.ptr != 0UL);
    
    std::uint32_t commandListCount = 0U;

    commandListCount += RecordAndPushPrePassCommandLists(frameBuffer);
        
    commandListCount += mCommandListRecorder.RecordAndPushCommandLists(frameBufferRenderTargetView);

    return commandListCount;
}

bool
PostProcessPass::IsDataValid() const noexcept
{
    const bool b =
        mInputColorBuffer != nullptr;

    return b;
}

std::uint32_t
PostProcessPass::RecordAndPushPrePassCommandLists(ID3D12Resource& frameBuffer) noexcept
{
    BRE_ASSERT(IsDataValid());

    D3D12_RESOURCE_BARRIER barriers[2U];
    std::uint32_t barrierCount = 0UL;
    if (ResourceStateManager::GetResourceState(*mInputColorBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mInputColorBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(frameBuffer) != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(frameBuffer,
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