#include "GeometryPass.h"

#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandManager.h>
#include <DescriptorManager\DescriptorManager.h>
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
	const DXGI_FORMAT sBufferFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN
	};

	void CreateBuffers(
		Microsoft::WRL::ComPtr<ID3D12Resource> buffers[GeometryPass::BUFFERS_COUNT],
		D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescs[GeometryPass::BUFFERS_COUNT]) noexcept {

		// Set shared buffers properties
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0U;
		resDesc.Width = SettingsManager::sWindowWidth;
		resDesc.Height = SettingsManager::sWindowHeight;
		resDesc.DepthOrArraySize = 1U;
		resDesc.MipLevels = 0U;
		resDesc.SampleDesc.Count = 1U;
		resDesc.SampleDesc.Quality = 0U;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

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
			resDesc.Format = sBufferFormats[i];

			clearValue[i].Format = resDesc.Format;

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = resDesc.Format;
			ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue[i], res);

			buffers[i] = Microsoft::WRL::ComPtr<ID3D12Resource>(res);
			DescriptorManager::Get().CreateRenderTargetView(*buffers[i].Get(), rtvDesc, &rtvCpuDescs[i]);
		}
	}

	void CreateCommandObjects(
		ID3D12CommandAllocator* cmdAllocs[SettingsManager::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &cmdList) noexcept {

		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(cmdList == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocs[i] == nullptr);
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocs[i]);
		}
		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocs[0], cmdList);
		cmdList->Close();
	}
}

void GeometryPass::Init(
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc,
	CommandListExecutor& cmdListExecutor,
	ID3D12CommandQueue& cmdQueue) noexcept {

	ASSERT(ValidateData() == false);
	
	ASSERT(mRecorders.empty() == false);

	mCmdListExecutor = &cmdListExecutor;
	mCmdQueue = &cmdQueue;

	CreateBuffers(mBuffers, mRtvCpuDescs);
	CreateCommandObjects(mCmdAllocs, mCmdList);

	mDepthBufferCpuDesc = depthBufferCpuDesc;

	// Initialize recorders PSOs
	ColorCmdListRecorder::InitPSO(sBufferFormats, BUFFERS_COUNT);
	ColorHeightCmdListRecorder::InitPSO(sBufferFormats, BUFFERS_COUNT);
	ColorNormalCmdListRecorder::InitPSO(sBufferFormats, BUFFERS_COUNT);
	HeightCmdListRecorder::InitPSO(sBufferFormats, BUFFERS_COUNT);
	NormalCmdListRecorder::InitPSO(sBufferFormats, BUFFERS_COUNT);
	TextureCmdListRecorder::InitPSO(sBufferFormats, BUFFERS_COUNT);

	// Build geometry buffers cpu descriptors
	const D3D12_CPU_DESCRIPTOR_HANDLE geomBuffersCpuDescs[]{
		mRtvCpuDescs[NORMAL_SMOOTHNESS],
		mRtvCpuDescs[BASECOLOR_METALMASK],
	};
	ASSERT(_countof(geomBuffersCpuDescs) == BUFFERS_COUNT);
	memcpy(mGeometryBuffersCpuDescs, &geomBuffersCpuDescs, sizeof(geomBuffersCpuDescs));

	// Init internal data for all geometry recorders
	for (Recorders::value_type& recorder : mRecorders) {
		ASSERT(recorder.get() != nullptr);
		recorder->InitInternal(
			mCmdListExecutor->CmdListQueue(),
			mGeometryBuffersCpuDescs,
			BUFFERS_COUNT,
			mDepthBufferCpuDesc);
	}

	ASSERT(ValidateData());
}

void GeometryPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {

	ASSERT(ValidateData());

	ExecuteBeginTask();

	const std::uint32_t taskCount{ static_cast<std::uint32_t>(mRecorders.size()) };
	mCmdListExecutor->ResetExecutedCmdListCount();

	// Execute geometry tasks
	std::uint32_t grainSize{ max(1U, (taskCount) / SettingsManager::sCpuProcessors) };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, taskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mRecorders[i]->RecordAndPushCommandLists(frameCBuffer);
	}
	);

	// Wait until all previous tasks command lists are executed
	while (mCmdListExecutor->ExecutedCmdListCount() < taskCount) {
		Sleep(0U);
	}
}

bool GeometryPass::ValidateData() const noexcept {
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAllocs[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
		if (mBuffers[i].Get() == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < BUFFERS_COUNT; ++i) {
		if (mRtvCpuDescs[i].ptr == 0UL) {
			return false;
		}
	}

	const bool b =
		mCmdListExecutor != nullptr &&
		mCmdQueue != nullptr &&
		mCmdList != nullptr &&
		mRecorders.empty() == false &&
		mDepthBufferCpuDesc.ptr != 0UL;

		return b;
}

void GeometryPass::ExecuteBeginTask() noexcept {
	ASSERT(ValidateData());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAlloc{ mCmdAllocs[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocs);

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, nullptr));

	mCmdList->RSSetViewports(1U, &SettingsManager::sScreenViewport);
	mCmdList->RSSetScissorRects(1U, &SettingsManager::sScissorRect);

	// Clear render targets and depth stencil
	float zero[4U] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCmdList->ClearRenderTargetView(mRtvCpuDescs[NORMAL_SMOOTHNESS], DirectX::Colors::Black, 0U, nullptr);
	mCmdList->ClearRenderTargetView(mRtvCpuDescs[BASECOLOR_METALMASK], zero, 0U, nullptr);
	mCmdList->ClearDepthStencilView(mDepthBufferCpuDesc, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0U, 0U, nullptr);

	CHECK_HR(mCmdList->Close());

	// Execute preliminary task
	ID3D12CommandList* cmdLists[] = { mCmdList };
	ASSERT(mCmdQueue != nullptr);
	mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}