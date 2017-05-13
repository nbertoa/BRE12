#include "LightingPass.h"

#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DXUtils/d3dx12.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

using namespace DirectX;

namespace BRE {
void
LightingPass::Init(ID3D12Resource& baseColorMetalMaskBuffer,
                   ID3D12Resource& normalSmoothnessBuffer,
                   ID3D12Resource& depthBuffer,
                   ID3D12Resource& diffuseIrradianceCubeMap,
                   ID3D12Resource& specularPreConvolvedCubeMap,
                   const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    mBaseColorMetalMaskBuffer = &baseColorMetalMaskBuffer;
    mNormalSmoothnessBuffer = &normalSmoothnessBuffer;
    mRenderTargetView = renderTargetView;
    mDepthBuffer = &depthBuffer;

    // Initialize ambient pass
    mEnvironmentLightPass.Init(*mBaseColorMetalMaskBuffer,
                               *mNormalSmoothnessBuffer,
                               depthBuffer,
                               diffuseIrradianceCubeMap,
                               specularPreConvolvedCubeMap,
                               renderTargetView);

    BRE_ASSERT(IsDataValid());
}

void
LightingPass::Execute(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(IsDataValid());

    ExecuteBeginTask();

    mEnvironmentLightPass.Execute(frameCBuffer);

    ExecuteFinalTask();
}

bool
LightingPass::IsDataValid() const noexcept
{

    const bool b =
        mBaseColorMetalMaskBuffer != nullptr &&
        mNormalSmoothnessBuffer != nullptr &&
        mRenderTargetView.ptr != 0UL &&
        mDepthBuffer != nullptr;

    return b;
}

void
LightingPass::ExecuteBeginTask() noexcept
{
    BRE_ASSERT(IsDataValid());

    // Check resource states:
    // - All geometry shaders must be in render target state because they were output
    // of the geometry pass.
#ifdef _DEBUG
    BRE_ASSERT(ResourceStateManager::GetResourceState(*mBaseColorMetalMaskBuffer) == D3D12_RESOURCE_STATE_RENDER_TARGET);
    BRE_ASSERT(ResourceStateManager::GetResourceState(*mNormalSmoothnessBuffer) == D3D12_RESOURCE_STATE_RENDER_TARGET);
#endif

    // - Depth buffer was used for depth testing in geometry pass, so it must be in depth write state
    BRE_ASSERT(ResourceStateManager::GetResourceState(*mDepthBuffer) == D3D12_RESOURCE_STATE_DEPTH_WRITE);

    ID3D12GraphicsCommandList& commandList = mBeginCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);

    CD3DX12_RESOURCE_BARRIER barriers[]
    {
        ResourceStateManager::ChangeResourceStateAndGetBarrier(*mBaseColorMetalMaskBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

        ResourceStateManager::ChangeResourceStateAndGetBarrier(*mNormalSmoothnessBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

        ResourceStateManager::ChangeResourceStateAndGetBarrier(*mDepthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    };

    const std::uint32_t barriersCount = _countof(barriers);
    BRE_ASSERT(barriersCount == 3UL);
    commandList.ResourceBarrier(barriersCount, barriers);

    commandList.ClearRenderTargetView(mRenderTargetView, Colors::Black, 0U, nullptr);

    BRE_CHECK_HR(commandList.Close());
    CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}

void
LightingPass::ExecuteFinalTask() noexcept
{
    BRE_ASSERT(IsDataValid());

    // Check resource states:
    // - All geometry shaders must be in pixel shader resource state because they were used
    // by lighting pass shaders.
#ifdef _DEBUG
    BRE_ASSERT(ResourceStateManager::GetResourceState(*mBaseColorMetalMaskBuffer) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    BRE_ASSERT(ResourceStateManager::GetResourceState(*mNormalSmoothnessBuffer) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
#endif

    // - Depth buffer must be in pixel shader resource because it was used by lighting pass shader.
    BRE_ASSERT(ResourceStateManager::GetResourceState(*mDepthBuffer) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    ID3D12GraphicsCommandList& commandList = mFinalCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);

    CD3DX12_RESOURCE_BARRIER barriers[]
    {
        ResourceStateManager::ChangeResourceStateAndGetBarrier(*mDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE),
    };

    const std::uint32_t barrierCount = _countof(barriers);
    BRE_ASSERT(barrierCount == 1UL);
    commandList.ResourceBarrier(barrierCount, barriers);

    BRE_CHECK_HR(commandList.Close());
    CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}
}

