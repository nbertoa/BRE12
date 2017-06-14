#include "ReflectionPass.h"

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
ReflectionPass::Init(ID3D12Resource& depthBuffer) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    mDepthBuffer = &depthBuffer;
    InitHierZBuffer();
    InitVisibilityBuffer();

    CopyResourcesCommandListRecorder::InitSharedPSOAndRootSignature();
    HiZBufferCommandListRecorder::InitSharedPSOAndRootSignature();
    VisibilityBufferCommandListRecorder::InitSharedPSOAndRootSignature();

    mCopyDepthBufferToHiZBufferMipLevel0CommandListRecorder.Init(mDepthBufferShaderResourceView,
                                                                 mHierZBufferMipLevelRenderTargetViews[0U]);

    for (std::uint32_t i = 0U; i < _countof(mHiZBufferCommandListRecorders); ++i) {
        mHiZBufferCommandListRecorders[i].Init(mHierZBufferMipLevelShaderResourceViews[i],
                                               mHierZBufferMipLevelRenderTargetViews[i + 1]);
    }

    for (std::uint32_t i = 0U; i < _countof(mVisibilityBufferCommandListRecorders); ++i) {
        mVisibilityBufferCommandListRecorders[i].Init(mHierZBufferMipLevelShaderResourceViews[i],
                                                      mVisibilityBufferMipLevelShaderResourceViews[i],
                                                      mVisibilityBufferMipLevelRenderTargetViews[i + 1]);
    }

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
ReflectionPass::Execute() noexcept
{
    BRE_ASSERT(IsDataValid());

    std::uint32_t commandListCount = 0U;

    commandListCount += RecordAndPushPrePassCommandLists();

    commandListCount += RecordAndPushHierZBufferCommandLists();

    commandListCount += RecordAndPushVisibilityBufferCommandLists();

    return commandListCount;
}

void
ReflectionPass::InitHierZBuffer() noexcept
{
    BRE_ASSERT(mHierZBuffer == nullptr);

    const std::uint32_t numMipLevels = _countof(mHierZBufferMipLevelRenderTargetViews);
    BRE_ASSERT(numMipLevels == _countof(mHierZBufferMipLevelShaderResourceViews));

    const DXGI_FORMAT bufferFormat = DXGI_FORMAT_R32G32_FLOAT;

    // Create hier z buffer     
    const D3D12_HEAP_PROPERTIES heapProperties = D3DFactory::GetHeapProperties();
    const D3D12_RESOURCE_DESC resourceDescriptor = D3DFactory::GetResourceDescriptor(ApplicationSettings::sWindowWidth,
                                                                                     ApplicationSettings::sWindowHeight,
                                                                                     bufferFormat,
                                                                                     D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                                                     D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                                                     D3D12_TEXTURE_LAYOUT_UNKNOWN,
                                                                                     numMipLevels);

    D3D12_CLEAR_VALUE clearValue{ resourceDescriptor.Format, 0.0f, 0.0f, 0.0f, 0.0f};

    mHierZBuffer = &ResourceManager::CreateCommittedResource(heapProperties,
                                                             D3D12_HEAP_FLAG_NONE,
                                                             resourceDescriptor,
                                                             D3D12_RESOURCE_STATE_COMMON,
                                                             &clearValue,
                                                             L"Hier Z Buffer",
                                                             ResourceManager::ResourceStateTrackingType::SUBRESOURCE_TRACKING);
    
    // Create shader resource views to each mip levels of the hier z buffer
    // Create render target views to each mip levels of the hier z buffer
    for (std::uint32_t i = 0U; i < numMipLevels; ++i) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
        srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDescriptor.Texture2D.MostDetailedMip = i;
        srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDescriptor.Format = mHierZBuffer->GetDesc().Format;
        srvDescriptor.Texture2D.MipLevels = mHierZBuffer->GetDesc().MipLevels - i;

        mHierZBufferMipLevelShaderResourceViews[i] = CbvSrvUavDescriptorManager::CreateShaderResourceView(*mHierZBuffer,
                                                                                                          srvDescriptor);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDescriptor{};
        rtvDescriptor.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDescriptor.Format = bufferFormat;
        rtvDescriptor.Texture2D.MipSlice = i;
        rtvDescriptor.Texture2D.PlaneSlice = 0U;
        RenderTargetDescriptorManager::CreateRenderTargetView(*mHierZBuffer,
                                                              rtvDescriptor,
                                                              &mHierZBufferMipLevelRenderTargetViews[i]);
    }

    // Create shader resource view to depth buffer
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = ApplicationSettings::sDepthStencilSRVFormat;
    srvDescriptor.Texture2D.MipLevels = mDepthBuffer->GetDesc().MipLevels;

    mDepthBufferShaderResourceView = CbvSrvUavDescriptorManager::CreateShaderResourceView(*mDepthBuffer,
                                                                                          srvDescriptor);
}

void
ReflectionPass::InitVisibilityBuffer() noexcept
{
    BRE_ASSERT(mVisibilityBuffer == nullptr);

    const std::uint32_t numMipLevels = _countof(mVisibilityBufferMipLevelRenderTargetViews);
    BRE_ASSERT(numMipLevels == _countof(mVisibilityBufferMipLevelShaderResourceViews));

    const DXGI_FORMAT bufferFormat = DXGI_FORMAT_R8_UNORM;

    // Create visibility buffer     
    const D3D12_HEAP_PROPERTIES heapProperties = D3DFactory::GetHeapProperties();
    const D3D12_RESOURCE_DESC resourceDescriptor = D3DFactory::GetResourceDescriptor(ApplicationSettings::sWindowWidth,
                                                                                     ApplicationSettings::sWindowHeight,
                                                                                     bufferFormat,
                                                                                     D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                                                     D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                                                     D3D12_TEXTURE_LAYOUT_UNKNOWN,
                                                                                     numMipLevels);

    D3D12_CLEAR_VALUE clearValue{ resourceDescriptor.Format, 1.0f, 1.0f, 1.0f, 1.0f };

    mVisibilityBuffer = &ResourceManager::CreateCommittedResource(heapProperties,
                                                                  D3D12_HEAP_FLAG_NONE,
                                                                  resourceDescriptor,
                                                                  D3D12_RESOURCE_STATE_COMMON,
                                                                  &clearValue,
                                                                  L"Visibility Buffer",
                                                                  ResourceManager::ResourceStateTrackingType::SUBRESOURCE_TRACKING);

    // Create shader resource views to each mip levels of the visibility buffer
    // Create render target views to each mip levels of the visibility buffer
    for (std::uint32_t i = 0U; i < numMipLevels; ++i) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
        srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDescriptor.Texture2D.MostDetailedMip = i;
        srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDescriptor.Format = mVisibilityBuffer->GetDesc().Format;
        srvDescriptor.Texture2D.MipLevels = mVisibilityBuffer->GetDesc().MipLevels - i;

        mVisibilityBufferMipLevelShaderResourceViews[i] = CbvSrvUavDescriptorManager::CreateShaderResourceView(*mVisibilityBuffer,
                                                                                                               srvDescriptor);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDescriptor{};
        rtvDescriptor.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDescriptor.Format = bufferFormat;
        rtvDescriptor.Texture2D.MipSlice = i;
        rtvDescriptor.Texture2D.PlaneSlice = 0U;
        RenderTargetDescriptorManager::CreateRenderTargetView(*mVisibilityBuffer,
                                                              rtvDescriptor,
                                                              &mVisibilityBufferMipLevelRenderTargetViews[i]);
    }
}

bool
ReflectionPass::IsDataValid() const noexcept
{
    const std::uint32_t numMipLevels = _countof(mHierZBufferMipLevelRenderTargetViews);
    BRE_ASSERT(numMipLevels == _countof(mHierZBufferMipLevelShaderResourceViews));

    for (std::uint32_t i = 0U; i < numMipLevels; ++i) {
        if (mHierZBufferMipLevelRenderTargetViews[i].ptr == 0UL) {
            return false;
        }

        if (mHierZBufferMipLevelShaderResourceViews[i].ptr == 0UL) {
            return false;
        }

        if (mVisibilityBufferMipLevelRenderTargetViews[i].ptr == 0UL) {
            return false;
        }

        if (mVisibilityBufferMipLevelShaderResourceViews[i].ptr == 0UL) {
            return false;
        }
    }

    const bool b =
        mHierZBuffer != nullptr &&
        mVisibilityBuffer != nullptr &&
        mDepthBuffer != nullptr;

    return b;
}

std::uint32_t
ReflectionPass::RecordAndPushPrePassCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());

    ID3D12GraphicsCommandList& commandList = mPrePassCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);

    const std::uint32_t numMipLevels = _countof(mHierZBufferMipLevelRenderTargetViews);

    // Barriers size = numMipLevels for hier-z buffer + numMipLevels for visibility buffer + depth buffer
    D3D12_RESOURCE_BARRIER barriers[numMipLevels * 2 + 1U];
    std::uint32_t barrierCount = 0UL;
    for (std::uint32_t i = 0U; i < numMipLevels; ++i) {
        if (ResourceStateManager::GetSubresourceState(*mHierZBuffer, i) != D3D12_RESOURCE_STATE_RENDER_TARGET) {
            barriers[barrierCount] = ResourceStateManager::ChangeSubresourceStateAndGetBarrier(*mHierZBuffer,
                                                                                               i,
                                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET);
            ++barrierCount;
        }

        if (ResourceStateManager::GetSubresourceState(*mVisibilityBuffer, i) != D3D12_RESOURCE_STATE_RENDER_TARGET) {
            barriers[barrierCount] = ResourceStateManager::ChangeSubresourceStateAndGetBarrier(*mVisibilityBuffer,
                                                                                               i,
                                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET);
            ++barrierCount;
        }
    }

    if (ResourceStateManager::GetResourceState(*mDepthBuffer) != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mDepthBuffer,
                                                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ++barrierCount;
    }

    if (barrierCount > 0UL) {
        commandList.ResourceBarrier(barrierCount, barriers);
    }

    // Clear all the mip levels of the hier z buffer
    float hierZBufferClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    for (std::uint32_t i = 0U; i < numMipLevels; ++i) {
        commandList.ClearRenderTargetView(mHierZBufferMipLevelRenderTargetViews[i],
                                          hierZBufferClearColor,
                                          0U,
                                          nullptr);
    }

    // Clear all the mip levels of the visibility buffer with the 100% visibility (1.0f)
    float fullVisibility[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    for (std::uint32_t i = 0U; i < numMipLevels; ++i) {
        commandList.ClearRenderTargetView(mVisibilityBufferMipLevelRenderTargetViews[1],
                                          fullVisibility,
                                          0U,
                                          nullptr);
    }

    BRE_CHECK_HR(commandList.Close());
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

std::uint32_t 
ReflectionPass::RecordAndPushHierZBufferCommandLists() noexcept
{
    std::uint32_t commandListCount = 0U;

    commandListCount += mCopyDepthBufferToHiZBufferMipLevel0CommandListRecorder.RecordAndPushCommandLists();

    for (std::uint32_t i = 0U; i < _countof(mHiZBufferCommandListRecorders); ++i) {
        commandListCount += mHiZBufferCommandListRecorders[i].RecordAndPushCommandLists();
    }

    return commandListCount;
}

std::uint32_t
ReflectionPass::RecordAndPushVisibilityBufferCommandLists() noexcept
{
    std::uint32_t commandListCount = 0U;

    for (std::uint32_t i = 0U; i < _countof(mVisibilityBufferCommandListRecorders); ++i) {
        commandListCount += mVisibilityBufferCommandListRecorders[i].RecordAndPushCommandLists();
    }

    return commandListCount;
}
}