#include "GeometryPass.h"

#include <cstdint>
#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListProcessor/CommandListProcessor.h>
#include <CommandManager\CommandManager.h>
#include <DXUtils\CBuffers.h>
#include <DXUtils/d3dx12.h>
#include <ResourceManager\ResourceManager.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateBuffers(
		ID3D12Device& device,
		Microsoft::WRL::ComPtr<ID3D12Resource> buffers[GeometryPass::BUFFERS_COUNT],
		D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescs[GeometryPass::BUFFERS_COUNT],
		ID3D12DescriptorHeap* &descHeap) noexcept {

		// Create buffers desc heap
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.NumDescriptors = GeometryPass::BUFFERS_COUNT;
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descHeapDesc.NodeMask = 0;
		ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, descHeap);

		// Set shared buffers properties
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0U;
		resDesc.Width = Settings::sWindowWidth;
		resDesc.Height = Settings::sWindowHeight;
		resDesc.DepthOrArraySize = 1U;
		resDesc.MipLevels = 0U;
		resDesc.SampleDesc.Count = 1U;
		resDesc.SampleDesc.Quality = 0U;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE clearValue[]{
			{ DXGI_FORMAT_UNKNOWN, 0.0f, 0.0f, 0.0f, 1.0f },
			{ DXGI_FORMAT_UNKNOWN, 0.0f, 0.0f, 0.0f, 0.0f },
			{ DXGI_FORMAT_UNKNOWN, 0.0f, 0.0f, 0.0f, 0.0f },
		};
		buffers[GeometryPass::NORMAL_SMOOTHNESS_DEPTH].Reset();
		buffers[GeometryPass::BASECOLOR_METALMASK].Reset();
		buffers[GeometryPass::SPECULARREFLECTION].Reset();

		CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };

		ID3D12Resource* res{ nullptr };
		D3D12_CPU_DESCRIPTOR_HANDLE currRtvCpuDesc(descHeap->GetCPUDescriptorHandleForHeapStart());
		const std::size_t rtvDescSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// Create and store RTV's descriptors for buffers
		for (std::uint32_t i = 0U; i < GeometryPass::BUFFERS_COUNT; ++i) {
			resDesc.Format = GeometryPass::BufferFormats()[i];
			clearValue[i].Format = resDesc.Format;
			rtvDesc.Format = resDesc.Format;
			ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, clearValue[i], res);
			buffers[i] = Microsoft::WRL::ComPtr<ID3D12Resource>(res);
			device.CreateRenderTargetView(buffers[i].Get(), &rtvDesc, currRtvCpuDesc);
			rtvCpuDescs[i] = currRtvCpuDesc;
			currRtvCpuDesc.ptr += rtvDescSize;
		}
	}

	void CreateCommandObjects(
		ID3D12CommandAllocator* cmdAllocs[Settings::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &cmdList) noexcept {

		ASSERT(Settings::sQueuedFrameCount > 0U);
		ASSERT(cmdList == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocs[i] == nullptr);
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocs[i]);
		}
		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocs[0], cmdList);
		cmdList->Close();
	}
}

const DXGI_FORMAT GeometryPass::sBufferFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{
	DXGI_FORMAT_R16G16B16A16_FLOAT,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_UNKNOWN
};

void GeometryPass::Init(ID3D12Device& device) noexcept {
	ASSERT(ValidateData() == false);
	
	ASSERT(mRecorders.empty() == false);

	CreateBuffers(device, mBuffers, mRtvCpuDescs, mDescHeap);
	CreateCommandObjects(mCmdAllocs, mCmdList);

	ASSERT(ValidateData());
}

void GeometryPass::Execute(
	CommandListProcessor& cmdListProcessor, 
	ID3D12CommandQueue& cmdQueue,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthCpuDesc,
	const FrameCBuffer& frameCBuffer) noexcept {

	ASSERT(ValidateData());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	std::uint32_t taskCount{ (std::uint32_t)mRecorders.size() };
	cmdListProcessor.ResetExecutedTasksCounter();

	ID3D12CommandAllocator* cmdAlloc{ mCmdAllocs[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocs);

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, nullptr));

	mCmdList->RSSetViewports(1U, &Settings::sScreenViewport);
	mCmdList->RSSetScissorRects(1U, &Settings::sScissorRect);

	// Clear render targets and depth stencil
	float zero[4U] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCmdList->ClearRenderTargetView(mRtvCpuDescs[NORMAL_SMOOTHNESS_DEPTH], DirectX::Colors::Black, 0U, nullptr);
	mCmdList->ClearRenderTargetView(mRtvCpuDescs[BASECOLOR_METALMASK], zero, 0U, nullptr);
	mCmdList->ClearRenderTargetView(mRtvCpuDescs[SPECULARREFLECTION], zero, 0U, nullptr);
	mCmdList->ClearDepthStencilView(depthCpuDesc, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0U, 0U, nullptr);
	CHECK_HR(mCmdList->Close());

	// Execute preliminary task
	ID3D12CommandList* cmdLists[] = { mCmdList };
	cmdQueue.ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// Build render targets cpu descriptors
	const D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescs[]{
		mRtvCpuDescs[NORMAL_SMOOTHNESS_DEPTH],
		mRtvCpuDescs[BASECOLOR_METALMASK],
		mRtvCpuDescs[SPECULARREFLECTION],
	};
	const std::uint32_t rtvCpuDescsCount = _countof(rtvCpuDescs);
	ASSERT(rtvCpuDescsCount == BUFFERS_COUNT);

	// Execute tasks
	std::uint32_t grainSize{ max(1U, (taskCount) / Settings::sCpuProcessors) };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, taskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mRecorders[i]->RecordCommandLists(frameCBuffer, rtvCpuDescs, rtvCpuDescsCount, depthCpuDesc);
	}
	);

	// Wait until all previous tasks command lists are executed
	while (cmdListProcessor.ExecutedTasksCounter() < taskCount) {
		Sleep(0U);
	}
}

bool GeometryPass::ValidateData() const noexcept {
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
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
		mCmdList != nullptr &&
		mRecorders.empty() == false &&
		mDescHeap != nullptr;

		return b;
}