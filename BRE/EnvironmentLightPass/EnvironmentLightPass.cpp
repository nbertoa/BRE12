#include "EnvironmentLightPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DescriptorManager\RenderTargetDescriptorManager.h>
#include <DXUtils\d3dx12.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <Utils\DebugUtils.h>

namespace BRE {
namespace {
///
/// @brief Creates resource and render target view for environment light pass
/// @param resourceInitialState Initial statea of the resource to create
/// @param resourceName Name of the resource
/// @param resource Output resource
/// @param resourceRenderTargetView Output render target view to the resource
///
void
CreateResourceAndRenderTargetView(const D3D12_RESOURCE_STATES resourceInitialState,
                                  const wchar_t* resourceName,
                                  ID3D12Resource* &resource,
                                  D3D12_CPU_DESCRIPTOR_HANDLE& resourceRenderTargetView) noexcept
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
}
}

void
EnvironmentLightPass::Init(ID3D12Resource& baseColorMetalMaskBuffer,
                           ID3D12Resource& normalSmoothnessBuffer,
                           ID3D12Resource& depthBuffer,
                           ID3D12Resource& diffuseIrradianceCubeMap,
                           ID3D12Resource& specularPreConvolvedCubeMap,
                           const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    AmbientOcclusionCommandListRecorder::InitSharedPSOAndRootSignature();
    BlurCommandListRecorder::InitSharedPSOAndRootSignature();
    EnvironmentLightCommandListRecorder::InitSharedPSOAndRootSignature();

    // Create ambient accessibility buffer and blur buffer
    CreateResourceAndRenderTargetView(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                      L"Ambient Accessibility Buffer",
                                      mAmbientAccessibilityBuffer,
                                      mAmbientAccessibilityBufferRenderTargetView);

    CreateResourceAndRenderTargetView(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                      L"Blur Buffer",
                                      mBlurBuffer,
                                      mBlurBufferRenderTargetView);

    // Initialize ambient occlusion recorder
    mAmbientOcclusionRecorder.reset(new AmbientOcclusionCommandListRecorder());
    mAmbientOcclusionRecorder->Init(normalSmoothnessBuffer,
                                    depthBuffer,
                                    mAmbientAccessibilityBufferRenderTargetView);

    // Initialize blur recorder
    mBlurRecorder.reset(new BlurCommandListRecorder());
    mBlurRecorder->Init(*mAmbientAccessibilityBuffer,
                        mBlurBufferRenderTargetView);

    // Initialize ambient light recorder
    mEnvironmentLightRecorder.reset(new EnvironmentLightCommandListRecorder());
    mEnvironmentLightRecorder->Init(normalSmoothnessBuffer,
                                    baseColorMetalMaskBuffer,
                                    depthBuffer,
                                    diffuseIrradianceCubeMap,
                                    specularPreConvolvedCubeMap,
                                    *mBlurBuffer,
                                    renderTargetView);

    mBaseColorMetalMaskBuffer = &baseColorMetalMaskBuffer;
    mNormalSmoothnessBuffer = &normalSmoothnessBuffer;
    mDepthBuffer = &depthBuffer;

    BRE_ASSERT(IsDataValid());
}

void
EnvironmentLightPass::Execute(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(IsDataValid());

    std::uint32_t taskCount{ 5U };
    CommandListExecutor::Get().ResetExecutedCommandListCount();

    ExecuteBeginTask();
    mAmbientOcclusionRecorder->RecordAndPushCommandLists(frameCBuffer);

    ExecuteMiddleTask();
    mBlurRecorder->RecordAndPushCommandLists();

    if (ExecuteFinalTask()) {
        ++taskCount;
    }
    mEnvironmentLightRecorder->RecordAndPushCommandLists(frameCBuffer);

    // Wait until all previous tasks command lists are executed
    while (CommandListExecutor::Get().GetExecutedCommandListCount() < taskCount) {
        Sleep(0U);
    }
}

bool
EnvironmentLightPass::IsDataValid() const noexcept
{
    const bool b =
        mAmbientOcclusionRecorder.get() != nullptr &&
        mEnvironmentLightRecorder.get() != nullptr &&
        mAmbientAccessibilityBuffer != nullptr &&
        mBlurBuffer != nullptr &&
        mBlurBufferRenderTargetView.ptr != 0UL &&
        mBaseColorMetalMaskBuffer != nullptr &&
        mNormalSmoothnessBuffer != nullptr &&
        mDepthBuffer != nullptr;

    return b;
}

void
EnvironmentLightPass::ExecuteBeginTask() noexcept
{
    BRE_ASSERT(IsDataValid());

    ID3D12GraphicsCommandList& commandList = mBeginCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);

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
    CommandListExecutor::Get().AddCommandList(commandList);
}

void
EnvironmentLightPass::ExecuteMiddleTask() noexcept
{
    BRE_ASSERT(IsDataValid());


    ID3D12GraphicsCommandList& commandList = mMiddleCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);

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
    CommandListExecutor::Get().AddCommandList(commandList);
}

bool
EnvironmentLightPass::ExecuteFinalTask() noexcept
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
        ID3D12GraphicsCommandList& commandList = mFinalCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);
        commandList.ResourceBarrier(barrierCount, barriers);
        BRE_CHECK_HR(commandList.Close());
        CommandListExecutor::Get().AddCommandList(commandList);

        return true;
    } else {
        return false;
    }
}
}