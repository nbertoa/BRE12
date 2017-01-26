#include "LightingPass.h"

#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <DXUtils/d3dx12.h>
#include <GeometryPass\GeometryPass.h>
#include <LightingPass\Recorders\PunctualLightCmdListRecorder.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateCommandObjects(
		ID3D12CommandAllocator* cmdAllocsBegin[SettingsManager::sQueuedFrameCount],
		ID3D12CommandAllocator* cmdAllocsEnd[SettingsManager::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &cmdList) noexcept {

		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(cmdList == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocsBegin[i] == nullptr);
			CommandAllocatorManager::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocsBegin[i]);

			ASSERT(cmdAllocsEnd[i] == nullptr);
			CommandAllocatorManager::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocsEnd[i]);
		}
		CommandListManager::Get().CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocsBegin[0], cmdList);
		cmdList->Close();
	}
}

void LightingPass::Init(
	CommandListExecutor& cmdListExecutor,
	ID3D12CommandQueue& cmdQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	ID3D12Resource& diffuseIrradianceCubeMap,
	ID3D12Resource& specularPreConvolvedCubeMap) noexcept {

	ASSERT(IsDataValid() == false);

	CreateCommandObjects(mCmdAllocsBegin, mCmdAllocsEnd, mCmdList);
	mCmdListExecutor = &cmdListExecutor;
	mCmdQueue = &cmdQueue;
	mGeometryBuffers = geometryBuffers;
	mColorBufferCpuDesc = colorBufferCpuDesc;
	mDepthBuffer = &depthBuffer;

	// Initialize recorder's pso
	PunctualLightCmdListRecorder::InitPSO();

	// Initialize ambient pass
	ASSERT(geometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get() != nullptr);
	mAmbientLightPass.Init(
		cmdListExecutor,
		cmdQueue,
		*geometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get(),
		*geometryBuffers[GeometryPass::NORMAL_SMOOTHNESS].Get(),
		colorBufferCpuDesc,
		depthBuffer);

	// Initialize environment light pass
	mEnvironmentLightPass.Init(
		cmdListExecutor.CmdListQueue(),
		geometryBuffers, 
		geometryBuffersCount,
		*mDepthBuffer,
		colorBufferCpuDesc, 
		diffuseIrradianceCubeMap,
		specularPreConvolvedCubeMap);

	// Init internal data for all lights recorders
	for (Recorders::value_type& recorder : mRecorders) {
		ASSERT(recorder.get() != nullptr);
		recorder->InitInternal(cmdListExecutor.CmdListQueue(), colorBufferCpuDesc);
	}

	ASSERT(IsDataValid());
}

void LightingPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());

	ExecuteBeginTask();

	// Total tasks = Light tasks + 1 ambient pass task + 1 environment light pass task
	mCmdListExecutor->ResetExecutedCmdListCount();
	const std::uint32_t lightTaskCount{ static_cast<std::uint32_t>(mRecorders.size())};
	
	// Execute light pass tasks
	const std::uint32_t grainSize(max(1U, lightTaskCount / SettingsManager::sCpuProcessorCount));
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, lightTaskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mRecorders[i]->RecordAndPushCommandLists(frameCBuffer);
	}
	);
	
	// Wait until all previous tasks command lists are executed
	while (mCmdListExecutor->ExecutedCmdListCount() < lightTaskCount) {
		Sleep(0U);
	}

	// Execute ambient light pass tasks
	mAmbientLightPass.Execute(frameCBuffer);

	// Execute environment light pass tasks
	//mEnvironmentLightPass.Execute(frameCBuffer);

	ExecuteEndingTask();
}

bool LightingPass::IsDataValid() const noexcept {
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAllocsBegin[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAllocsEnd[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < GeometryPass::BUFFERS_COUNT; ++i) {
		if (mGeometryBuffers[i].Get() == nullptr) {
			return false;
		}
	}

	const bool b =
		mCmdListExecutor != nullptr &&
		mCmdQueue != nullptr &&
		mCmdList != nullptr &&
		mColorBufferCpuDesc.ptr != 0UL &&
		mDepthBuffer != nullptr;

	return b;
}

void LightingPass::ExecuteBeginTask() noexcept {
	ASSERT(IsDataValid());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAllocBegin{ mCmdAllocsBegin[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocsBegin);

	CHECK_HR(cmdAllocBegin->Reset());
	CHECK_HR(mCmdList->Reset(cmdAllocBegin, nullptr));

	// Resource barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		ResourceStateManager::Get().ChangeResourceStateAndGetBarrier(*mGeometryBuffers[GeometryPass::NORMAL_SMOOTHNESS].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::Get().ChangeResourceStateAndGetBarrier(*mGeometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::Get().ChangeResourceStateAndGetBarrier(*mDepthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};
	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == GeometryPass::BUFFERS_COUNT + 1UL);
	mCmdList->ResourceBarrier(barriersCount, barriers);

	// Clear render targets
	mCmdList->ClearRenderTargetView(mColorBufferCpuDesc, DirectX::Colors::Black, 0U, nullptr);
	CHECK_HR(mCmdList->Close());

	// Execute preliminary task
	{
		ID3D12CommandList* cmdLists[] = { mCmdList };
		mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}
}

void LightingPass::ExecuteEndingTask() noexcept {
	ASSERT(IsDataValid());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAllocEnd{ mCmdAllocsEnd[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocsBegin);

	// Prepare end task
	CHECK_HR(cmdAllocEnd->Reset());
	CHECK_HR(mCmdList->Reset(cmdAllocEnd, nullptr));

	// Resource barriers
	CD3DX12_RESOURCE_BARRIER endBarriers[]{
		ResourceStateManager::Get().ChangeResourceStateAndGetBarrier(*mDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE),
	};
	const std::uint32_t barriersCount = _countof(endBarriers);
	ASSERT(barriersCount == 1UL);
	mCmdList->ResourceBarrier(barriersCount, endBarriers);
	CHECK_HR(mCmdList->Close());

	// Execute end task
	{
		ID3D12CommandList* cmdLists[] = { mCmdList };
		mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}
}