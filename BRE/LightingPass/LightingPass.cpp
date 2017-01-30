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
		ID3D12CommandAllocator* cmdAllocatorsBegin[SettingsManager::sQueuedFrameCount],
		ID3D12CommandAllocator* cmdAllocatorsEnd[SettingsManager::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &commandList) noexcept 
	{
		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(commandList == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocatorsBegin[i] == nullptr);
			CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocatorsBegin[i]);

			ASSERT(cmdAllocatorsEnd[i] == nullptr);
			CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocatorsEnd[i]);
		}
		CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocatorsBegin[0], commandList);
		commandList->Close();
	}
}

void LightingPass::Init(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	ID3D12Resource& diffuseIrradianceCubeMap,
	ID3D12Resource& specularPreConvolvedCubeMap) noexcept 
{
	ASSERT(IsDataValid() == false);

	CreateCommandObjects(mCmdAllocatorsBegin, mCmdAllocatorsFinal, mCommandList);
	mGeometryBuffers = geometryBuffers;
	mOutputColorBufferCpuDesc = colorBufferCpuDesc;
	mDepthBuffer = &depthBuffer;

	// Initialize recorder's pso
	PunctualLightCmdListRecorder::InitPSO();

	// Initialize ambient pass
	ASSERT(geometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get() != nullptr);
	mAmbientLightPass.Init(
		*geometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get(),
		*geometryBuffers[GeometryPass::NORMAL_SMOOTHNESS].Get(),
		colorBufferCpuDesc,
		depthBuffer);

	// Initialize environment light pass
	mEnvironmentLightPass.Init(
		geometryBuffers, 
		geometryBuffersCount,
		*mDepthBuffer,
		colorBufferCpuDesc, 
		diffuseIrradianceCubeMap,
		specularPreConvolvedCubeMap);

	for (CommandListRecorders::value_type& recorder : mRecorders) {
		ASSERT(recorder.get() != nullptr);
		recorder->SetOutputColorBufferCpuDescriptor(colorBufferCpuDesc);
	}

	ASSERT(IsDataValid());
}

void LightingPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());

	ExecuteBeginTask();

	// Total tasks = Light tasks + 1 ambient pass task + 1 environment light pass task
	CommandListExecutor::Get().ResetExecutedCommandListCount();
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
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < lightTaskCount) {
		Sleep(0U);
	}

	// Execute ambient light pass tasks
	mAmbientLightPass.Execute(frameCBuffer);

	// Execute environment light pass tasks
	mEnvironmentLightPass.Execute(frameCBuffer);

	ExecuteFinalTask();
}

bool LightingPass::IsDataValid() const noexcept {
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAllocatorsBegin[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAllocatorsFinal[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < GeometryPass::BUFFERS_COUNT; ++i) {
		if (mGeometryBuffers[i].Get() == nullptr) {
			return false;
		}
	}

	const bool b =
		mCommandList != nullptr &&
		mOutputColorBufferCpuDesc.ptr != 0UL &&
		mDepthBuffer != nullptr;

	return b;
}

void LightingPass::ExecuteBeginTask() noexcept {
	ASSERT(IsDataValid());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAllocBegin{ mCmdAllocatorsBegin[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocatorsBegin);

	CHECK_HR(cmdAllocBegin->Reset());
	CHECK_HR(mCommandList->Reset(cmdAllocBegin, nullptr));

	// GetResource barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mGeometryBuffers[GeometryPass::NORMAL_SMOOTHNESS].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mGeometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mDepthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};
	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == GeometryPass::BUFFERS_COUNT + 1UL);
	mCommandList->ResourceBarrier(barriersCount, barriers);

	// Clear render targets
	mCommandList->ClearRenderTargetView(mOutputColorBufferCpuDesc, DirectX::Colors::Black, 0U, nullptr);
	CHECK_HR(mCommandList->Close());

	// Execute preliminary task
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(*mCommandList);
}

void LightingPass::ExecuteFinalTask() noexcept {
	ASSERT(IsDataValid());

	// Used to choose a different command list allocator each call.
	static std::uint32_t commandAllocatorIndex{ 0U };

	ID3D12CommandAllocator* commandAllocatorEnd{ mCmdAllocatorsFinal[commandAllocatorIndex] };
	commandAllocatorIndex = (commandAllocatorIndex + 1U) % _countof(mCmdAllocatorsBegin);

	// Prepare end task
	CHECK_HR(commandAllocatorEnd->Reset());
	CHECK_HR(mCommandList->Reset(commandAllocatorEnd, nullptr));

	// GetResource barriers
	CD3DX12_RESOURCE_BARRIER endBarriers[]{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE),
	};
	const std::uint32_t barriersCount = _countof(endBarriers);
	ASSERT(barriersCount == 1UL);
	mCommandList->ResourceBarrier(barriersCount, endBarriers);
	CHECK_HR(mCommandList->Close());

	// Execute final task
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(*mCommandList);
}