#include "LightPass.h"

#include <cstdint>
#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListProcessor/CommandListProcessor.h>
#include <CommandManager\CommandManager.h>
#include <DXUtils\CBuffers.h>
#include <DXUtils/d3dx12.h>
#include <GeometryPass\GeometryPass.h>
#include <LightPass\Recorders\PunctualLightCmdListRecorder.h>
#include <Utils\DebugUtils.h>

namespace {
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

void LightPass::Init(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept {

	ASSERT(ValidateData() == false);

	ASSERT(mRecorders.empty() == false);

	CreateCommandObjects(mCmdAllocs, mCmdList);
	mGeometryBuffers = geometryBuffers;
	mColorBufferCpuDesc = colorBufferCpuDesc;
	mDepthBufferCpuDesc = depthBufferCpuDesc;

	// Initialize recorder's pso
	PunctualLightCmdListRecorder::InitPSO();

	ASSERT(ValidateData());
}

void LightPass::Execute(
	CommandListProcessor& cmdListProcessor,
	ID3D12CommandQueue& cmdQueue,
	const FrameCBuffer& frameCBuffer) noexcept {

	ASSERT(ValidateData());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAlloc{ mCmdAllocs[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocs);

	const std::uint32_t taskCount{ (std::uint32_t)mRecorders.size() };
	cmdListProcessor.ResetExecutedTasksCounter();

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, nullptr));

	// Resource barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryBuffers[GeometryPass::NORMAL_SMOOTHNESS_DEPTH].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryBuffers[GeometryPass::SPECULARREFLECTION].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};
	const std::size_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == GeometryPass::BUFFERS_COUNT);
	mCmdList->ResourceBarrier(barriersCount, barriers);

	// Clear render targets
	mCmdList->ClearRenderTargetView(mColorBufferCpuDesc, DirectX::Colors::Black, 0U, nullptr);
	CHECK_HR(mCmdList->Close());

	// Execute preliminary task
	ID3D12CommandList* cmdLists[] = { mCmdList };
	cmdQueue.ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// Execute light pass tasks
	const std::uint32_t grainSize(max(1U, taskCount / Settings::sCpuProcessors));
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescs[] = { mColorBufferCpuDesc };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, taskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mRecorders[i]->RecordCommandLists(frameCBuffer, rtvCpuDescs, _countof(rtvCpuDescs), mDepthBufferCpuDesc);
	}
	);

	// Wait until all previous tasks command lists are executed
	while (cmdListProcessor.ExecutedTasksCounter() < taskCount) {
		Sleep(0U);
	}
}

bool LightPass::ValidateData() const noexcept {
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		if (mCmdAllocs[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < GeometryPass::BUFFERS_COUNT; ++i) {
		if (mGeometryBuffers[i].Get() == nullptr) {
			return false;
		}
	}

	const bool b =
		mCmdList != nullptr &&
		mRecorders.empty() == false &&
		mColorBufferCpuDesc.ptr != 0UL &&
		mDepthBufferCpuDesc.ptr != 0UL;

	return b;
}