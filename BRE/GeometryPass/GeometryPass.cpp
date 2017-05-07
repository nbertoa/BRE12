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

using namespace DirectX;

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
		D3D12_CPU_DESCRIPTOR_HANDLE bufferRenderTargetViews[GeometryPass::BUFFERS_COUNT]) noexcept 
	{
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

			D3D12_RENDER_TARGET_VIEW_DESC rtvDescriptor{};
			rtvDescriptor.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDescriptor.Format = resourceDescriptor.Format;
			ID3D12Resource* resource = &ResourceManager::CreateCommittedResource(
				heapProps, 
				D3D12_HEAP_FLAG_NONE, 
				resourceDescriptor, 
				D3D12_RESOURCE_STATE_RENDER_TARGET, 
				&clearValue[i],
				resourceNames[i]);

			buffers[i] = Microsoft::WRL::ComPtr<ID3D12Resource>(resource);
			RenderTargetDescriptorManager::CreateRenderTargetView(*buffers[i].Get(), rtvDescriptor, &bufferRenderTargetViews[i]);
		}
	}
}

GeometryPass::GeometryPass(GeometryPassCommandListRecorders& geometryPassCommandListRecorders)
	: mGeometryPassCommandListRecorders(geometryPassCommandListRecorders)
{}

void GeometryPass::Init(const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept {
	ASSERT(IsDataValid() == false);
	
	ASSERT(mGeometryPassCommandListRecorders.empty() == false);

	CreateGeometryBuffersAndRenderTargetViews(mGeometryBuffers, mGeometryBufferRenderTargetViews);

	mDepthBufferView = depthBufferView;

	ColorCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	ColorHeightCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	ColorNormalCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	HeightCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	NormalCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	TextureCmdListRecorder::InitSharedPSOAndRootSignature(sGeometryBufferFormats, BUFFERS_COUNT);
	
	// Init geometry command list recorders
	for (GeometryPassCommandListRecorders::value_type& recorder : mGeometryPassCommandListRecorders) {
		ASSERT(recorder.get() != nullptr);
		recorder->Init(mGeometryBufferRenderTargetViews, BUFFERS_COUNT, mDepthBufferView);
	}

	ASSERT(IsDataValid());
}

void GeometryPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());

	ExecuteBeginTask();

	const std::uint32_t taskCount{ static_cast<std::uint32_t>(mGeometryPassCommandListRecorders.size()) };
	CommandListExecutor::Get().ResetExecutedCommandListCount();

	// Execute tasks
	std::uint32_t grainSize{ max(1U, (taskCount) / SettingsManager::sCpuProcessorCount) };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, taskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mGeometryPassCommandListRecorders[i]->RecordAndPushCommandLists(frameCBuffer);
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
		if (mGeometryBufferRenderTargetViews[i].ptr == 0UL) {
			return false;
		}
	}

	const bool b =
		mGeometryPassCommandListRecorders.empty() == false &&
		mDepthBufferView.ptr != 0UL;

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

	float zero[4U] = { 0.0f, 0.0f, 0.0f, 0.0f };
	commandList.ClearRenderTargetView(mGeometryBufferRenderTargetViews[NORMAL_SMOOTHNESS], Colors::Black, 0U, nullptr);
	commandList.ClearRenderTargetView(mGeometryBufferRenderTargetViews[BASECOLOR_METALMASK], zero, 0U, nullptr);
	commandList.ClearDepthStencilView(mDepthBufferView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0U, 0U, nullptr);

	CHECK_HR(commandList.Close());
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}