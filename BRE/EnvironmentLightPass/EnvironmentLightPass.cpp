#include "EnvironmentLightPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateCommandObjects(
		ID3D12CommandAllocator* &commandAllocators,
		ID3D12GraphicsCommandList* &commandList) noexcept {

		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(commandList == nullptr);

		// Create command allocator and command list
		commandAllocators = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
		commandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators);
		commandList->Close();
	}
}

void EnvironmentLightPass::Init(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc,
	ID3D12Resource& diffuseIrradianceCubeMap,
	ID3D12Resource& specularPreConvolvedCubeMap) noexcept 
{
	ASSERT(ValidateData() == false);
	
	CreateCommandObjects(mCommandAllocators, mCommandList);

	// Initialize recorder's PSO
	EnvironmentLightCmdListRecorder::InitPSO();

	// Initialize recorder
	mRecorder.reset(new EnvironmentLightCmdListRecorder());
	mRecorder->Init(
		geometryBuffers, 
		geometryBuffersCount,
		depthBuffer,
		outputColorBufferCpuDesc,
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
		mCommandAllocators != nullptr &&
		mCommandList != nullptr &&
		mRecorder.get() != nullptr;

	return b;
}