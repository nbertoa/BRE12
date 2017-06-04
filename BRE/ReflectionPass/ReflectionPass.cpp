#include "ReflectionPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DescriptorManager\RenderTargetDescriptorManager.h>
#include <DXUtils\d3dx12.h>
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

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
ReflectionPass::Execute() noexcept
{
    BRE_ASSERT(IsDataValid());

    std::uint32_t commandListCount = 0U;

    commandListCount += RecordAndPushPrePassCommandLists();
    commandListCount += RecordAndPushHierZBufferCommandLists();

    return commandListCount;
}

void
ReflectionPass::InitHierZBuffer() noexcept
{
    BRE_ASSERT(mHierZBuffer == nullptr);

    const std::uint32_t numMipLevels = _countof(mHierZBufferMipLevelRenderTargetViews);
    BRE_ASSERT(numMipLevels == _countof(mHierZBufferMipLevelShaderResourceViews));

    // Create hier z buffer     
    CD3DX12_HEAP_PROPERTIES heapProperties{ D3D12_HEAP_TYPE_DEFAULT };
    D3D12_RESOURCE_DESC resourceDescriptor = {};
    resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescriptor.Alignment = 0U;
    resourceDescriptor.Width = ApplicationSettings::sWindowWidth;
    resourceDescriptor.Height = ApplicationSettings::sWindowHeight;
    resourceDescriptor.DepthOrArraySize = 1U;
    resourceDescriptor.MipLevels = numMipLevels;
    resourceDescriptor.SampleDesc.Count = 1U;
    resourceDescriptor.SampleDesc.Quality = 0U;
    resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    resourceDescriptor.Format = ApplicationSettings::sDepthStencilSRVFormat;

    D3D12_CLEAR_VALUE clearValue{ resourceDescriptor.Format, 0.0f};

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
        rtvDescriptor.Format = ApplicationSettings::sDepthStencilSRVFormat;
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
    }

    const bool b =
        mHierZBuffer != nullptr &&
        mDepthBuffer != nullptr;

    return b;
}

std::uint32_t
ReflectionPass::RecordAndPushPrePassCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());

    ID3D12GraphicsCommandList& commandList = mPrePassCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);

    const std::uint32_t numMipLevels = _countof(mHierZBufferMipLevelRenderTargetViews);

    CD3DX12_RESOURCE_BARRIER barriers[numMipLevels + 1U];
    std::uint32_t barrierCount = 0UL;
    for (std::uint32_t i = 0; i < numMipLevels; ++i) {
        if (ResourceStateManager::GetSubresourceState(*mHierZBuffer, i) != D3D12_RESOURCE_STATE_RENDER_TARGET) {
            barriers[barrierCount] = ResourceStateManager::ChangeSubresourceStateAndGetBarrier(*mHierZBuffer,
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
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList.ClearRenderTargetView(mHierZBufferMipLevelRenderTargetViews[0],
                                      clearColor,
                                      0U,
                                      nullptr);

    BRE_CHECK_HR(commandList.Close());
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

std::uint32_t 
ReflectionPass::RecordAndPushHierZBufferCommandLists() noexcept
{
    return 0U;
}
}