#include "GeometryPass.h"

#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DescriptorManager\RenderTargetDescriptorManager.h>
#include <DXUtils/d3dx12.h>
#include <GeometryPass\Recorders\ColorCmdListRecorder.h>
#include <GeometryPass\Recorders\ColorHeightCmdListRecorder.h>
#include <GeometryPass\Recorders\ColorNormalCmdListRecorder.h>
#include <GeometryPass\Recorders\HeightCmdListRecorder.h>
#include <GeometryPass\Recorders\NormalCmdListRecorder.h>
#include <GeometryPass\Recorders\TextureCmdListRecorder.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

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

	void CreateGeometryBuffersAndRenderTargetViews(
		Microsoft::WRL::ComPtr<ID3D12Resource> buffers[GeometryPass::BUFFERS_COUNT],
		D3D12_CPU_DESCRIPTOR_HANDLE bufferRenderTargetViewCpuDescriptors[GeometryPass::BUFFERS_COUNT]) noexcept {

		// Set shared buffers properties
		D3D12_RESOURCE_DESC resourceDescriptor = {};
		resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDescriptor.Alignment = 0U;
		resourceDescriptor.Width = SettingsManager::sWindowWidth;
		resourceDescriptor.Height = SettingsManager::sWindowHeight;
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
		ASSERT(_countof(clearValue) == GeometryPass::BUFFERS_COUNT);

		buffers[GeometryPass::NORMAL_SMOOTHNESS].Reset();
		buffers[GeometryPass::BASECOLOR_METALMASK].Reset();

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

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = resourceDescriptor.Format;
			ID3D12Resource* resource = &ResourceManager::CreateCommittedResource(
				heapProps, 
				D3D12_HEAP_FLAG_NONE, 
				resourceDescriptor, 
				D3D12_RESOURCE_STATE_RENDER_TARGET, 
				&clearValue[i],
				resourceNames[i]);

			buffers[i] = Microsoft::WRL::ComPtr<ID3D12Resource>(resource);
			RenderTargetDescriptorManager::CreateRenderTargetView(*buffers[i].Get(), rtvDesc, &bufferRenderTargetViewCpuDescriptors[i]);
		}
	}
}

void GeometryPass::Init(const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept 
{
	ASSERT(IsDataValid() == false);
	
	ASSERT(mCommandListRecorders.empty() == false);

	CreateGeometryBuffersAndRenderTargetViews(mGeometryBuffers, mGeometryBufferRenderTargetCpuDescriptors);

	mDepthBufferCpuDescriptor = depthBufferCpuDesc;

	// Initialize recorders PSOs
	ColorCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	ColorHeightCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	ColorNormalCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	HeightCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	NormalCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	TextureCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	
	// Init internal data for all geometry recorders
	for (CommandListRecorders::value_type& recorder : mCommandListRecorders) {
		ASSERT(recorder.get() != nullptr);
		recorder->Init(mGeometryBufferRenderTargetCpuDescriptors, BUFFERS_COUNT, mDepthBufferCpuDescriptor);
	}

	ASSERT(IsDataValid());
}

void GeometryPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());

	ExecuteBeginTask();

	const std::uint32_t taskCount{ static_cast<std::uint32_t>(mCommandListRecorders.size()) };
	CommandListExecutor::Get().ResetExecutedCommandListCount();

	// Execute geometry tasks
	std::uint32_t grainSize{ max(1U, (taskCount) / SettingsManager::sCpuProcessorCount) };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, taskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mCommandListRecorders[i]->RecordAndPushCommandLists(frameCBuffer);
	}
	);

	// Wait until all previous tasks command lists are executed
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < taskCount) {
		Sleep(0U);
	}
}

bool GeometryPass::IsDataValid() const noexcept {
	for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
		if (mGeometryBuffers[i].Get() == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
		if (mGeometryBufferRenderTargetCpuDescriptors[i].ptr == 0UL) {
			return false;
		}
	}

	const bool b =
		mCommandListRecorders.empty() == false &&
		mDepthBufferCpuDescriptor.ptr != 0UL;

		return b;
}

void GeometryPass::ExecuteBeginTask() noexcept {
	ASSERT(IsDataValid());

	// Check resource states:
	// As geometry pass is the first pass, then all geometry buffers must be 
	// in render target state because final pass changed them.
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
		ASSERT(ResourceStateManager::GetResourceState(*mGeometryBuffers[i].Get()) == D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
#endif

	ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetWithNextCommandAllocator(nullptr);

	commandList.RSSetViewports(1U, &SettingsManager::sScreenViewport);
	commandList.RSSetScissorRects(1U, &SettingsManager::sScissorRect);

	// Clear render targets and depth stencil
	float zero[4U] = { 0.0f, 0.0f, 0.0f, 0.0f };
	commandList.ClearRenderTargetView(mGeometryBufferRenderTargetCpuDescriptors[NORMAL_SMOOTHNESS], DirectX::Colors::Black, 0U, nullptr);
	commandList.ClearRenderTargetView(mGeometryBufferRenderTargetCpuDescriptors[BASECOLOR_METALMASK], zero, 0U, nullptr);
	commandList.ClearDepthStencilView(mDepthBufferCpuDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0U, 0U, nullptr);

	CHECK_HR(commandList.Close());
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}