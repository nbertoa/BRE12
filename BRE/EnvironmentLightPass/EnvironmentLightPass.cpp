#include "EnvironmentLightPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateCommandObjects(
		ID3D12CommandAllocator* &cmdAlloc,
		ID3D12GraphicsCommandList* &cmdList) noexcept {

		ASSERT(Settings::sQueuedFrameCount > 0U);
		ASSERT(cmdList == nullptr);

		// Create command allocator and command list
		CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc);
		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc, cmdList);
		cmdList->Close();
	}
}

void EnvironmentLightPass::Init(
	ID3D12Device& device,
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	ID3D12Resource& diffuseIrradianceCubeMap,
	ID3D12Resource& specularPreConvolvedCubeMap) noexcept {

	ASSERT(ValidateData() == false);
	
	CreateCommandObjects(mCmdAlloc, mCmdList);

	// Initialize recorder's PSO
	EnvironmentLightCmdListRecorder::InitPSO();

	// Initialize recorder
	mRecorder.reset(new EnvironmentLightCmdListRecorder(device, cmdListQueue));
	mRecorder->Init(
		geometryBuffers, 
		geometryBuffersCount,
		depthBuffer,
		colorBufferCpuDesc,
		diffuseIrradianceCubeMap,
		specularPreConvolvedCubeMap);

	ASSERT(ValidateData());
}

void EnvironmentLightPass::Execute(const FrameCBuffer& frameCBuffer) const noexcept {
	ASSERT(ValidateData());

	mRecorder->RecordAndPushCommandLists(frameCBuffer);
}

bool EnvironmentLightPass::ValidateData() const noexcept {
	const bool b =
		mCmdAlloc != nullptr &&
		mCmdList != nullptr &&
		mRecorder.get() != nullptr;

	return b;
}