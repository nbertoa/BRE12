#include "GeometryPass.h"

#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <DescriptorManager\RenderTargetDescriptorManager.h>
#include <DXUtils/d3dx12.h>
#include <GeometryPass\Recorders\ColorCmdListRecorder.h>
#include <GeometryPass\Recorders\ColorHeightCmdListRecorder.h>
#include <GeometryPass\Recorders\ColorNormalCmdListRecorder.h>
#include <GeometryPass\Recorders\HeightCmdListRecorder.h>
#include <GeometryPass\Recorders\NormalCmdListRecorder.h>
#include <GeometryPass\Recorders\TextureCmdListRecorder.h>
#include <ResourceManager\ResourceManager.h>
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

	void CreateGeometryBuffers(
		Microsoft::WRL::ComPtr<ID3D12Resource> buffers[GeometryPass::BUFFERS_COUNT],
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewCpuDescs[GeometryPass::BUFFERS_COUNT]) noexcept {

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

		D3D12_CLEAR_VALUE clearValue[]{
			{ DXGI_FORMAT_UNKNOWN, 0.0f, 0.0f, 0.0f, 1.0f },
			{ DXGI_FORMAT_UNKNOWN, 0.0f, 0.0f, 0.0f, 0.0f },
		};
		ASSERT(_countof(clearValue) == GeometryPass::BUFFERS_COUNT);

		buffers[GeometryPass::NORMAL_SMOOTHNESS].Reset();
		buffers[GeometryPass::BASECOLOR_METALMASK].Reset();

		CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };

		ID3D12Resource* res{ nullptr };

		// Create and store RTV's descriptors for buffers
		for (std::uint32_t i = 0U; i < GeometryPass::BUFFERS_COUNT; ++i) {
			resourceDescriptor.Format = sGeometryBufferFormats[i];

			clearValue[i].Format = resourceDescriptor.Format;

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = resourceDescriptor.Format;
			ResourceManager::CreateCommittedResource(
				heapProps, 
				D3D12_HEAP_FLAG_NONE, 
				resourceDescriptor, 
				D3D12_RESOURCE_STATE_RENDER_TARGET, 
				&clearValue[i], 
				res);

			buffers[i] = Microsoft::WRL::ComPtr<ID3D12Resource>(res);
			RenderTargetDescriptorManager::CreateRenderTargetView(*buffers[i].Get(), rtvDesc, &renderTargetViewCpuDescs[i]);
		}
	}

	void CreateCommandObjects(
		ID3D12CommandAllocator* commandAllocators[SettingsManager::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &commandList) noexcept {

		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(commandList == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(commandAllocators[i] == nullptr);
			CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[i]);
		}
		CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators[0], commandList);
		commandList->Close();
	}
}

void GeometryPass::Init(const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept 
{
	ASSERT(IsDataValid() == false);
	
	ASSERT(mRecorders.empty() == false);

	CreateGeometryBuffers(mGeometryBuffers, mGeometryBufferRenderTargetCpuDescs);
	CreateCommandObjects(mCommandAllocators, mCommandList);

	mDepthBufferCpuDesc = depthBufferCpuDesc;

	// Initialize recorders PSOs
	ColorCmdListRecorder::InitPSO(sGeometryBufferFormats, BUFFERS_COUNT);
	ColorHeightCmdListRecorder::InitPSO(sGeometryBufferFormats, BUFFERS_COUNT);
	ColorNormalCmdListRecorder::InitPSO(sGeometryBufferFormats, BUFFERS_COUNT);
	HeightCmdListRecorder::InitPSO(sGeometryBufferFormats, BUFFERS_COUNT);
	NormalCmdListRecorder::InitPSO(sGeometryBufferFormats, BUFFERS_COUNT);
	TextureCmdListRecorder::InitPSO(sGeometryBufferFormats, BUFFERS_COUNT);

	// Build geometry buffers cpu descriptors
	const D3D12_CPU_DESCRIPTOR_HANDLE geometryBuffersCpuDescs[]{
		mGeometryBufferRenderTargetCpuDescs[NORMAL_SMOOTHNESS],
		mGeometryBufferRenderTargetCpuDescs[BASECOLOR_METALMASK],
	};
	ASSERT(_countof(geometryBuffersCpuDescs) == BUFFERS_COUNT);
	memcpy(mGeometryBuffersCpuDescs, &geometryBuffersCpuDescs, sizeof(geometryBuffersCpuDescs));

	// Init internal data for all geometry recorders
	for (CommandListRecorders::value_type& recorder : mRecorders) {
		ASSERT(recorder.get() != nullptr);
		recorder->Init(mGeometryBuffersCpuDescs, BUFFERS_COUNT, mDepthBufferCpuDesc);
	}

	ASSERT(IsDataValid());
}

void GeometryPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());

	ExecuteBeginTask();

	const std::uint32_t taskCount{ static_cast<std::uint32_t>(mRecorders.size()) };
	CommandListExecutor::Get().ResetExecutedCommandListCount();

	// Execute geometry tasks
	std::uint32_t grainSize{ max(1U, (taskCount) / SettingsManager::sCpuProcessorCount) };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, taskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mRecorders[i]->RecordAndPushCommandLists(frameCBuffer);
	}
	);

	// Wait until all previous tasks command lists are executed
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < taskCount) {
		Sleep(0U);
	}
}

bool GeometryPass::IsDataValid() const noexcept {
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocators[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
		if (mGeometryBuffers[i].Get() == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
		if (mGeometryBufferRenderTargetCpuDescs[i].ptr == 0UL) {
			return false;
		}
	}

	const bool b =
		mCommandList != nullptr &&
		mRecorders.empty() == false &&
		mDepthBufferCpuDesc.ptr != 0UL;

		return b;
}

void GeometryPass::ExecuteBeginTask() noexcept {
	ASSERT(IsDataValid());

	// Used to choose a different command list allocator each call.
	static std::uint32_t commandAllocatorIndex{ 0U };

	ID3D12CommandAllocator* commandAllocator{ mCommandAllocators[commandAllocatorIndex] };
	commandAllocatorIndex = (commandAllocatorIndex + 1U) % _countof(mCommandAllocators);

	CHECK_HR(commandAllocator->Reset());
	CHECK_HR(mCommandList->Reset(commandAllocator, nullptr));

	mCommandList->RSSetViewports(1U, &SettingsManager::sScreenViewport);
	mCommandList->RSSetScissorRects(1U, &SettingsManager::sScissorRect);

	// Clear render targets and depth stencil
	float zero[4U] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCommandList->ClearRenderTargetView(mGeometryBufferRenderTargetCpuDescs[NORMAL_SMOOTHNESS], DirectX::Colors::Black, 0U, nullptr);
	mCommandList->ClearRenderTargetView(mGeometryBufferRenderTargetCpuDescs[BASECOLOR_METALMASK], zero, 0U, nullptr);
	mCommandList->ClearDepthStencilView(mDepthBufferCpuDesc, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0U, 0U, nullptr);

	CHECK_HR(mCommandList->Close());

	// Execute preliminary task
	CommandListExecutor::Get().ResetExecutedCommandListCount();
	CommandListExecutor::Get().AddCommandList(*mCommandList);

	// Wait until all previous tasks command lists are executed
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < 1) {
		Sleep(0U);
	}
}