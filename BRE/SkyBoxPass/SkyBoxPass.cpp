#include "SkyBoxPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <CommandManager\FenceManager.h>
#include <DXUtils\DXUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\Model.h>
#include <ModelManager\ModelManager.h>
#include <SkyBoxPass\SkyBoxCmdListRecorder.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateCommandObjects(
		ID3D12CommandAllocator* &commandAllocators,
		ID3D12GraphicsCommandList* &commandList,
		ID3D12Fence* &fence) noexcept 
	{
		ASSERT(commandAllocators == nullptr);
		ASSERT(commandList == nullptr);
		ASSERT(fence == nullptr);

		// Create command allocators and command list
		CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators);
		CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators, commandList);
		commandList->Close();

		FenceManager::CreateFence(0U, D3D12_FENCE_FLAG_NONE, fence);
	}
}

void SkyBoxPass::Init(
	ID3D12CommandQueue& commandQueue,
	ID3D12Resource& skyBoxCubeMap,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept 
{
	ASSERT(IsDataValid() == false);

	CreateCommandObjects(mCommandAllocators, mCommandList, mFence);

	CHECK_HR(mCommandList->Reset(mCommandAllocators, nullptr));

	// Create sky box sphere
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::CreateSphere(3000, 50, 50, model, *mCommandList, uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);
	const std::vector<Mesh>& meshes(model->GetMeshes());
	ASSERT(meshes.size() == 1UL);

	// Build world matrix
	const Mesh& mesh{ meshes[0] };
	DirectX::XMFLOAT4X4 worldMatrix;
	MathUtils::ComputeMatrix(worldMatrix, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	
	DXUtils::ExecuteCommandListAndWaitForCompletion(commandQueue, *mCommandList, *mFence);

	// Initialize recoders's pso
	SkyBoxCmdListRecorder::InitPSO();

	// Initialize recorder
	mRecorder.reset(new SkyBoxCmdListRecorder());
	mRecorder->Init(
		mesh.GetVertexBufferData(),
		mesh.GetIndexBufferData(), 
		worldMatrix, 
		skyBoxCubeMap,
		outputColorBufferCpuDesc,
		depthBufferCpuDesc);

	ASSERT(IsDataValid());
}

void SkyBoxPass::Execute(const FrameCBuffer& frameCBuffer) const noexcept {
	ASSERT(IsDataValid());

	CommandListExecutor::Get().ResetExecutedCommandListCount();
	mRecorder->RecordAndPushCommandLists(frameCBuffer);

	// Wait until all previous tasks command lists are executed
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < 1U) {
		Sleep(0U);
	}
}

bool SkyBoxPass::IsDataValid() const noexcept {
	const bool b =
		mRecorder.get() != nullptr &&
		mCommandAllocators != nullptr &&
		mCommandList != nullptr &&
		mFence != nullptr;

	return b;
}