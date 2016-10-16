#include "LightPass.h"

#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListProcessor/CommandListProcessor.h>
#include <CommandManager\CommandManager.h>
#include <DXUtils/d3dx12.h>
#include <GeometryPass\GeometryPass.h>
#include <LightPass\Recorders\PunctualLightCmdListRecorder.h>
#include <ShaderUtils\CBuffers.h>
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
	ID3D12Device& device,
	ID3D12CommandQueue& cmdQueue,
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
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

	// Initialize ambient pass
	ASSERT(geometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get() != nullptr);
	mAmbientPass.Init(
		device,
		cmdQueue,
		cmdListQueue,
		*geometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get(),
		colorBufferCpuDesc,
		depthBufferCpuDesc);

	// Initialize environment light pass
	mEnvironmentLightPass.Init(
		device, 
		cmdQueue, 
		cmdListQueue, 
		geometryBuffers, 
		GeometryPass::BUFFERS_COUNT, 
		colorBufferCpuDesc, 
		depthBufferCpuDesc);

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

	// Total tasks = Light tasks + 1 ambient pass task + 1 environment light pass task
	cmdListProcessor.ResetExecutedTasksCounter();
	//const std::uint32_t lightTaskCount{ static_cast<std::uint32_t>(mRecorders.size())};
	//const std::uint32_t taskCount{ lightTaskCount + 2U };

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, nullptr));

	// Resource barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryBuffers[GeometryPass::NORMAL_SMOOTHNESS_DEPTH].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryBuffers[GeometryPass::DIFFUSEREFLECTION].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
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

	// Execute ambient light pass tasks
	//mAmbientPass.Execute();

	// Execute environment light pass tasks
	mEnvironmentLightPass.Execute(frameCBuffer);

	// Execute light pass tasks
	/*const std::uint32_t grainSize(max(1U, lightTaskCount / Settings::sCpuProcessors));
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescs[] = { mColorBufferCpuDesc };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, lightTaskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mRecorders[i]->RecordCommandLists(frameCBuffer, rtvCpuDescs, _countof(rtvCpuDescs), mDepthBufferCpuDesc);
	}
	);*/

	// Wait until all previous tasks command lists are executed
	while (cmdListProcessor.ExecutedTasksCounter() < 1) {
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