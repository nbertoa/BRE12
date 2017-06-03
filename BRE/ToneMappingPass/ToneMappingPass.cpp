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
                      ID3D12Resource& outputColorBuffer,
                      const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    mInputColorBuffer = &inputColorBuffer;
    mOutputColorBuffer = &outputColorBuffer;

    ToneMappingCommandListRecorder::InitSharedPSOAndRootSignature();

    mCommandListRecorder.reset(new ToneMappingCommandListRecorder());
    mCommandListRecorder->Init(*mInputColorBuffer, renderTargetView);

    BRE_ASSERT(IsDataValid());
}

void
ToneMappingPass::Execute() noexcept
{
    BRE_ASSERT(IsDataValid());

    std::uint32_t commandListCount = 1;
    CommandListExecutor::Get().ResetExecutedCommandListCount();

    if (RecordPrePassCommandList()) {
        ++commandListCount;
    }

    mCommandListRecorder->RecordAndPushCommandLists();

    // Wait until all previous tasks command lists are executed
    while (CommandListExecutor::Get().GetExecutedCommandListCount() < commandListCount) {
        Sleep(0U);
    }
}

bool
ToneMappingPass::IsDataValid() const noexcept
{
    const bool b =
        mCommandListRecorder.get() != nullptr &&
        mInputColorBuffer != nullptr &&
        mOutputColorBuffer != nullptr;

    return b;
}

bool
ToneMappingPass::RecordPrePassCommandList() noexcept
{
    BRE_ASSERT(IsDataValid());

    CD3DX12_RESOURCE_BARRIER barriers[2U];
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
        CommandListExecutor::Get().AddCommandList(commandList);

        return true;
    }

    return false;
}
}