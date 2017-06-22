#include "EnvironmentLightPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DescriptorManager\RenderTargetDescriptorManager.h>
#include <DXUtils\D3DFactory.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <Utils\DebugUtils.h>

namespace BRE {
void
EnvironmentLightPass::Init(ID3D12Resource& baseColorMetalnessBuffer,
                           ID3D12Resource& normalRoughnessBuffer,
                           ID3D12Resource& depthBuffer,
                           ID3D12Resource& diffuseIrradianceCubeMap,
                           ID3D12Resource& specularPreConvolvedCubeMap,
                           ID3D12Resource& ambientAccessibilityBuffer,
                           const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferRenderTargetView,
                           const D3D12_GPU_DESCRIPTOR_HANDLE& geometryBufferShaderResourceViewsBegin,
                           const D3D12_GPU_DESCRIPTOR_HANDLE& ambientAccessibilityBufferShaderResourceView,
                           const D3D12_GPU_DESCRIPTOR_HANDLE& depthBufferShaderResourceView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    EnvironmentLightCommandListRecorder::InitSharedPSOAndRootSignature();

    // Initialize ambient light recorder
    mEnvironmentLightRecorder.Init(diffuseIrradianceCubeMap,
                                   specularPreConvolvedCubeMap,
                                   outputColorBufferRenderTargetView,
                                   geometryBufferShaderResourceViewsBegin,
                                   ambientAccessibilityBufferShaderResourceView,
                                   depthBufferShaderResourceView);

    mBaseColorMetalnessBuffer = &baseColorMetalnessBuffer;
    mNormalRoughnessBuffer = &normalRoughnessBuffer;
    mDepthBuffer = &depthBuffer;
    mAmbientAccessibilityBuffer = &ambientAccessibilityBuffer;

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
EnvironmentLightPass::Execute(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(IsDataValid());

    std::uint32_t commandListCount = 0U;

    commandListCount += RecordAndPushPrePassCommandLists();
    commandListCount += mEnvironmentLightRecorder.RecordAndPushCommandLists(frameCBuffer);

    return commandListCount;
}

bool
EnvironmentLightPass::IsDataValid() const noexcept
{
    const bool b =
        mAmbientAccessibilityBuffer != nullptr &&
        mBaseColorMetalnessBuffer != nullptr &&
        mNormalRoughnessBuffer != nullptr &&
        mDepthBuffer != nullptr;

    return b;
}

std::uint32_t
EnvironmentLightPass::RecordAndPushPrePassCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());

    D3D12_RESOURCE_BARRIER barriers[5U];
    std::uint32_t barrierCount = 0UL;
    if (ResourceStateManager::GetResourceState(*mAmbientAccessibilityBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mAmbientAccessibilityBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(*mBaseColorMetalnessBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mBaseColorMetalnessBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(*mNormalRoughnessBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mNormalRoughnessBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(*mDepthBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mDepthBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(*mAmbientAccessibilityBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mAmbientAccessibilityBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (barrierCount > 0UL) {
        ID3D12GraphicsCommandList& commandList = mPostPassCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);
        commandList.ResourceBarrier(barrierCount, barriers);
        BRE_CHECK_HR(commandList.Close());
        CommandListExecutor::Get().PushCommandList(commandList);

        return 1U;
    }

    return 0U;
}
}