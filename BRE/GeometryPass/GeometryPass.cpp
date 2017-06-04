#include "GeometryPass.h"

#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DescriptorManager\RenderTargetDescriptorManager.h>
#include <DirectXManager\DirectXManager.h>
#include <DXUtils/d3dx12.h>
#include <GeometryPass\Recorders\ColorMappingCommandListRecorder.h>
#include <GeometryPass\Recorders\ColorHeightMappingCommandListRecorder.h>
#include <GeometryPass\Recorders\ColorNormalMappingCommandListRecorder.h>
#include <GeometryPass\Recorders\HeightMappingCommandListRecorder.h>
#include <GeometryPass\Recorders\NormalMappingCommandListRecorder.h>
#include <GeometryPass\Recorders\TextureMappingCommandListRecorder.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

using namespace DirectX;

namespace BRE {
namespace {
// Geometry buffer formats
const DXGI_FORMAT sGeometryBufferFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN
};

///
/// @brief Create geometry buffers and render target views
/// @param buffers Output list of geometry buffers
/// @param bufferRenderTargetViews Output geometry buffers render target views
///
void
CreateGeometryBuffersAndRenderTargetViews(ID3D12Resource* buffers[GeometryPass::BUFFERS_COUNT],
                                          D3D12_CPU_DESCRIPTOR_HANDLE bufferRenderTargetViews[GeometryPass::BUFFERS_COUNT]) noexcept
{
    // Set shared buffers properties
    D3D12_RESOURCE_DESC resourceDescriptor = {};
    resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescriptor.Alignment = 0U;
    resourceDescriptor.Width = ApplicationSettings::sWindowWidth;
    resourceDescriptor.Height = ApplicationSettings::sWindowHeight;
    resourceDescriptor.DepthOrArraySize = 1U;
    resourceDescriptor.MipLevels = 0U;
    resourceDescriptor.SampleDesc.Count = 1U;
    resourceDescriptor.SampleDesc.Quality = 0U;
    resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearValue[]
    {
        { DXGI_FORMAT_UNKNOWN, 0.0f, 0.0f, 0.0f, 1.0f },
        { DXGI_FORMAT_UNKNOWN, 0.0f, 0.0f, 0.0f, 0.0f },
    };
    BRE_ASSERT(_countof(clearValue) == GeometryPass::BUFFERS_COUNT);

    CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };

    // Create and store render target views
    const wchar_t* resourceNames[GeometryPass::BUFFERS_COUNT] =
    {
        L"Normal_SmoothnessTexture Buffer",
        L"BaseColor_MetalMaskTexture Buffer"
    };
    for (std::uint32_t i = 0U; i < GeometryPass::BUFFERS_COUNT; ++i) {
        resourceDescriptor.Format = sGeometryBufferFormats[i];

        clearValue[i].Format = resourceDescriptor.Format;

        D3D12_RENDER_TARGET_VIEW_DESC rtvDescriptor{};
        rtvDescriptor.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDescriptor.Format = resourceDescriptor.Format;
        resourceDescriptor.MipLevels = 1U;
        buffers[i] = &ResourceManager::CreateCommittedResource(heapProps,
                                                               D3D12_HEAP_FLAG_NONE,
                                                               resourceDescriptor,
                                                               D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                               &clearValue[i],
                                                               resourceNames[i],
                                                               ResourceManager::ResourceStateTrackingType::FULL_TRACKING);

        RenderTargetDescriptorManager::CreateRenderTargetView(*buffers[i],
                                                              rtvDescriptor,
                                                              &bufferRenderTargetViews[i]);
    }
}
}

GeometryPass::GeometryPass(GeometryCommandListRecorders& geometryPassCommandListRecorders)
    : mGeometryCommandListRecorders(geometryPassCommandListRecorders)
{}

void
GeometryPass::Init(const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    BRE_ASSERT(mGeometryCommandListRecorders.empty() == false);

    CreateGeometryBuffersAndRenderTargetViews(mGeometryBuffers, mGeometryBufferRenderTargetViews);

    ColorMappingCommandListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
    ColorHeightMappingCommandListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
    ColorNormalMappingCommandListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
    HeightMappingCommandListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
    NormalMappingCommandListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
    TextureMappingCommandListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);

    InitShaderResourceViews();

    // Init geometry command list recorders
    for (GeometryCommandListRecorders::value_type& recorder : mGeometryCommandListRecorders) {
        BRE_ASSERT(recorder.get() != nullptr);
        recorder->Init(mGeometryBufferRenderTargetViews,
                       BUFFERS_COUNT,
                       depthBufferView);
    }

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
GeometryPass::Execute(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(IsDataValid());

    const std::uint32_t geometryPassCommandListCount = static_cast<std::uint32_t>(mGeometryCommandListRecorders.size());
    std::uint32_t commandListCount = geometryPassCommandListCount;

    commandListCount += RecordAndPushPrePassCommandLists();

    // Execute tasks
    std::uint32_t grainSize{ max(1U, (geometryPassCommandListCount) / ApplicationSettings::sCpuProcessorCount) };
    tbb::parallel_for(tbb::blocked_range<std::size_t>(0, geometryPassCommandListCount, grainSize),
                      [&](const tbb::blocked_range<size_t>& r) {
        for (size_t i = r.begin(); i != r.end(); ++i)
            mGeometryCommandListRecorders[i]->RecordAndPushCommandLists(frameCBuffer);
    }
    );

    return commandListCount;
}

bool
GeometryPass::IsDataValid() const noexcept
{
    for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
        if (mGeometryBuffers[i] == nullptr) {
            return false;
        }
    }

    for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
        if (mGeometryBufferRenderTargetViews[i].ptr == 0UL) {
            return false;
        }
    }

    return mGeometryCommandListRecorders.empty() == false;
}

std::uint32_t
GeometryPass::RecordAndPushPrePassCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());

    ID3D12GraphicsCommandList& commandList = mPrePassCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);

    CD3DX12_RESOURCE_BARRIER barriers[BUFFERS_COUNT];
    std::uint32_t barrierCount = 0UL;
    for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
        if (ResourceStateManager::GetResourceState(*mGeometryBuffers[i]) != D3D12_RESOURCE_STATE_RENDER_TARGET) {
            barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mGeometryBuffers[i],
                                                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
            ++barrierCount;
        }
    }

    if (barrierCount > 0UL) {
        commandList.ResourceBarrier(barrierCount, barriers);
    }

    commandList.RSSetViewports(1U, &ApplicationSettings::sScreenViewport);
    commandList.RSSetScissorRects(1U, &ApplicationSettings::sScissorRect);

    float zero[4U] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList.ClearRenderTargetView(mGeometryBufferRenderTargetViews[NORMAL_SMOOTHNESS], Colors::Black, 0U, nullptr);
    commandList.ClearRenderTargetView(mGeometryBufferRenderTargetViews[BASECOLOR_METALMASK], zero, 0U, nullptr);

    BRE_CHECK_HR(commandList.Close());
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

void
GeometryPass::InitShaderResourceViews() noexcept
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptors[BUFFERS_COUNT]{};

    for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
        srvDescriptors[i].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDescriptors[i].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDescriptors[i].Texture2D.MostDetailedMip = 0;
        srvDescriptors[i].Texture2D.ResourceMinLODClamp = 0.0f;
        srvDescriptors[i].Format = mGeometryBuffers[i]->GetDesc().Format;
        srvDescriptors[i].Texture2D.MipLevels = mGeometryBuffers[i]->GetDesc().MipLevels;
    }
    
    const D3D12_GPU_DESCRIPTOR_HANDLE shaderResourceViewBegin = 
        CbvSrvUavDescriptorManager::CreateShaderResourceViews(mGeometryBuffers,
                                                              srvDescriptors,
                                                              BUFFERS_COUNT);

    // After creating all the contiguous descriptors, we need to initialize each
    // shader resource view member variables
    const std::size_t descriptorHandleIncrementSize = 
        DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
        mGeometryBufferShaderResourceViews[i].ptr = shaderResourceViewBegin.ptr + i * descriptorHandleIncrementSize;
    }

}
}