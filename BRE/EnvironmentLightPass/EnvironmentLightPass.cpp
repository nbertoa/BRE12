#include "EnvironmentLightPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DescriptorManager\RenderTargetDescriptorManager.h>
#include <DXUtils\d3dx12.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <Utils\DebugUtils.h>

namespace BRE {
namespace {
///
/// @brief Creates resource, render target view and shader resource view.
/// @param resourceInitialState Initial statea of the resource to create
/// @param resourceName Name of the resource
/// @param resource Output resource
/// @param resourceRenderTargetView Output render target view to the resource
///
void
CreateResourceAndRenderTargetView(const D3D12_RESOURCE_STATES resourceInitialState,
                                  const wchar_t* resourceName,
                                  ID3D12Resource* &resource,
                                  D3D12_CPU_DESCRIPTOR_HANDLE& resourceRenderTargetView,
                                  D3D12_GPU_DESCRIPTOR_HANDLE& resourceShaderResourceView) noexcept
{
    BRE_ASSERT(resource == nullptr);

    // Set shared buffers properties
    D3D12_RESOURCE_DESC resourceDescriptor = {};
    resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescriptor.Alignment = 0U;
    resourceDescriptor.Width = ApplicationSettings::sWindowWidth;
    resourceDescriptor.Height = ApplicationSettings::sWindowHeight;
    resourceDescriptor.DepthOrArraySize = 1U;
    resourceDescriptor.MipLevels = 1U;
    resourceDescriptor.SampleDesc.Count = 1U;
    resourceDescriptor.SampleDesc.Quality = 0U;
    resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    resourceDescriptor.Format = DXGI_FORMAT_R16_UNORM;

    D3D12_CLEAR_VALUE clearValue{ resourceDescriptor.Format, 0.0f, 0.0f, 0.0f, 0.0f };

    CD3DX12_HEAP_PROPERTIES heapProperties{ D3D12_HEAP_TYPE_DEFAULT };

    // Create buffer resource
    resource = &ResourceManager::CreateCommittedResource(heapProperties,
                                                         D3D12_HEAP_FLAG_NONE,
                                                         resourceDescriptor,
                                                         resourceInitialState,
                                                         &clearValue,
                                                         resourceName,
                                                         ResourceManager::ResourceStateTrackingType::FULL_TRACKING);

    // Create render target view	
    D3D12_RENDER_TARGET_VIEW_DESC rtvDescriptor{};
    rtvDescriptor.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDescriptor.Format = resourceDescriptor.Format;
    RenderTargetDescriptorManager::CreateRenderTargetView(*resource,
                                                          rtvDescriptor,
                                                          &resourceRenderTargetView);

    // Create shader resource view
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = resource->GetDesc().Format;
    srvDescriptor.Texture2D.MipLevels = resource->GetDesc().MipLevels;
    resourceShaderResourceView = CbvSrvUavDescriptorManager::CreateShaderResourceView(*resource,
                                                                                      srvDescriptor);
}
}

void
EnvironmentLightPass::Init(ID3D12Resource& baseColorMetalMaskBuffer,
                           ID3D12Resource& normalSmoothnessBuffer,
                           ID3D12Resource& depthBuffer,
                           ID3D12Resource& diffuseIrradianceCubeMap,
                           ID3D12Resource& specularPreConvolvedCubeMap,
                           const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferRenderTargetView,
                           const D3D12_GPU_DESCRIPTOR_HANDLE& depthBufferShaderResourceView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    AmbientOcclusionCommandListRecorder::InitSharedPSOAndRootSignature();
    BlurCommandListRecorder::InitSharedPSOAndRootSignature();
    EnvironmentLightCommandListRecorder::InitSharedPSOAndRootSignature();

    // Create ambient accessibility buffer and blur buffer
    CreateResourceAndRenderTargetView(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                      L"Ambient Accessibility Buffer",
                                      mAmbientAccessibilityBuffer,
                                      mAmbientAccessibilityBufferRenderTargetView,
                                      mAmbientAccessibilityBufferShaderResourceView);

    CreateResourceAndRenderTargetView(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                      L"Blur Buffer",
                                      mBlurBuffer,
                                      mBlurBufferRenderTargetView,
                                      mBlurBufferShaderResourceView);

    // Initialize ambient occlusion recorder
    mAmbientOcclusionRecorder.reset(new AmbientOcclusionCommandListRecorder());
    mAmbientOcclusionRecorder->Init(normalSmoothnessBuffer,
                                    mAmbientAccessibilityBufferRenderTargetView,
                                    depthBufferShaderResourceView);

    // Initialize blur recorder
    mBlurRecorder.reset(new BlurCommandListRecorder());
    mBlurRecorder->Init(mAmbientAccessibilityBufferShaderResourceView,
                        mBlurBufferRenderTargetView);

    // Initialize ambient light recorder
    mEnvironmentLightRecorder.reset(new EnvironmentLightCommandListRecorder());
    mEnvironmentLightRecorder->Init(normalSmoothnessBuffer,
                                    baseColorMetalMaskBuffer,
                                    diffuseIrradianceCubeMap,
                                    specularPreConvolvedCubeMap,
                                    *mBlurBuffer,
                                    outputColorBufferRenderTargetView,
                                    depthBufferShaderResourceView);

    mBaseColorMetalMaskBuffer = &baseColorMetalMaskBuffer;
    mNormalSmoothnessBuffer = &normalSmoothnessBuffer;
    mDepthBuffer = &depthBuffer;

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
EnvironmentLightPass::Execute(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(IsDataValid());

    std::uint32_t commandListCount = 0U;

    commandListCount += RecordAndPushPrePassCommandLists();
    commandListCount += mAmbientOcclusionRecorder->RecordAndPushCommandLists(frameCBuffer);

    commandListCount += RecordAndPushMiddlePassCommandLists();
    commandListCount += mBlurRecorder->RecordAndPushCommandLists();

    commandListCount += RecordAndPushPostPassCommandLists();
    commandListCount += mEnvironmentLightRecorder->RecordAndPushCommandLists(frameCBuffer);

    return commandListCount;
}

bool
EnvironmentLightPass::IsDataValid() const noexcept
{
    const bool b =
        mAmbientOcclusionRecorder.get() != nullptr &&
        mEnvironmentLightRecorder.get() != nullptr &&
        mAmbientAccessibilityBuffer != nullptr &&
        mAmbientAccessibilityBufferShaderResourceView.ptr != 0UL &&
        mAmbientAccessibilityBufferRenderTargetView.ptr != 0UL &&
        mBlurBuffer != nullptr &&
        mBlurBufferShaderResourceView.ptr != 0UL &&
        mBlurBufferRenderTargetView.ptr != 0UL &&
        mBaseColorMetalMaskBuffer != nullptr &&
        mNormalSmoothnessBuffer != nullptr &&
        mDepthBuffer != nullptr;

    return b;
}

std::uint32_t
EnvironmentLightPass::RecordAndPushPrePassCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());

    ID3D12GraphicsCommandList& commandList = mPrePassCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);

    CD3DX12_RESOURCE_BARRIER barriers[5U];
    std::uint32_t barrierCount = 0UL;
    if (ResourceStateManager::GetResourceState(*mAmbientAccessibilityBuffer) != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mAmbientAccessibilityBuffer,
                                                                                        D3D12_RESOURCE_STATE_RENDER_TARGET);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(*mBlurBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mBlurBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(*mBaseColorMetalMaskBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mBaseColorMetalMaskBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(*mNormalSmoothnessBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mNormalSmoothnessBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(*mDepthBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mDepthBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (barrierCount > 0UL) {
        commandList.ResourceBarrier(barrierCount, barriers);
    }

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList.ClearRenderTargetView(mAmbientAccessibilityBufferRenderTargetView,
                                      clearColor,
                                      0U,
                                      nullptr);

    BRE_CHECK_HR(commandList.Close());
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

std::uint32_t
EnvironmentLightPass::RecordAndPushMiddlePassCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());


    ID3D12GraphicsCommandList& commandList = mMiddlePassCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);

    CD3DX12_RESOURCE_BARRIER barriers[2U];
    std::uint32_t barrierCount = 0UL;
    if (ResourceStateManager::GetResourceState(*mAmbientAccessibilityBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mAmbientAccessibilityBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (ResourceStateManager::GetResourceState(*mBlurBuffer) != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mBlurBuffer,
                                                                                        D3D12_RESOURCE_STATE_RENDER_TARGET);
        ++barrierCount;
    }

    if (barrierCount > 0UL) {
        commandList.ResourceBarrier(barrierCount, barriers);
    }

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList.ClearRenderTargetView(mBlurBufferRenderTargetView,
                                      clearColor,
                                      0U,
                                      nullptr);

    BRE_CHECK_HR(commandList.Close());
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

std::uint32_t
EnvironmentLightPass::RecordAndPushPostPassCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());

    CD3DX12_RESOURCE_BARRIER barriers[1U];
    std::uint32_t barrierCount = 0UL;
    if (ResourceStateManager::GetResourceState(*mBlurBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mBlurBuffer,
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