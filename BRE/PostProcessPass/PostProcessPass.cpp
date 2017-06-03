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
PostProcessPass::Init(ID3D12Resource& inputColorBuffer) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    mInputColorBuffer = &inputColorBuffer;

    PostProcessCommandListRecorder::InitSharedPSOAndRootSignature();

    mCommandListRecorder.reset(new PostProcessCommandListRecorder());
    mCommandListRecorder->Init(inputColorBuffer);

    BRE_ASSERT(IsDataValid());
}

void
PostProcessPass::Execute(ID3D12Resource& renderTargetBuffer,
                         const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept
{
    BRE_ASSERT(IsDataValid());
    BRE_ASSERT(renderTargetView.ptr != 0UL);
    
    std::uint32_t commandListCount = 1;
    CommandListExecutor::Get().ResetExecutedCommandListCount();

    if (RecordPrePassCommandList(renderTargetBuffer,
                                  renderTargetView)) {
        ++commandListCount;
    }
        
    mCommandListRecorder->RecordAndPushCommandLists(renderTargetView);

    // Wait until all previous tasks command lists are executed
    while (CommandListExecutor::Get().GetExecutedCommandListCount() < commandListCount) {
        Sleep(0U);
    }
}

bool
PostProcessPass::IsDataValid() const noexcept
{
    const bool b =
        mCommandListRecorder.get() != nullptr &&
        mInputColorBuffer != nullptr;

    return b;
}

bool
PostProcessPass::RecordPrePassCommandList(ID3D12Resource& renderTargetBuffer,
                                           const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept
{
    BRE_ASSERT(IsDataValid());
    BRE_ASSERT(renderTargetView.ptr != 0UL);

    CD3DX12_RESOURCE_BARRIER barriers[2U];
    std::uint32_t barrierCount = 0UL;
    if (ResourceStateManager::GetResourceState(*mInputColorBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mInputColorBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(renderTargetBuffer) != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(renderTargetBuffer,
                                                                                        D3D12_RESOURCE_STATE_RENDER_TARGET);
        ++barrierCount;
    }

    if (barrierCount > 0UL) {
        ID3D12GraphicsCommandList& commandList = mPrePassCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);
        commandList.ResourceBarrier(barrierCount, barriers);
        BRE_CHECK_HR(commandList.Close());
        CommandListExecutor::Get().AddCommandList(commandList);

        return true;
    }

    return false;
}
}