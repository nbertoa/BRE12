#include "SkyBoxPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <DXUtils\DXUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\Model.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <SkyBoxPass\SkyBoxCmdListRecorder.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateCommandObjects(
		ID3D12CommandAllocator* &cmdAlloc,
		ID3D12GraphicsCommandList* &cmdList,
		ID3D12Fence* &fence) noexcept {

		ASSERT(cmdAlloc == nullptr);
		ASSERT(cmdList == nullptr);
		ASSERT(fence == nullptr);

		// Create command allocators and command list
		CommandAllocatorManager::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc);
		CommandListManager::Get().CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc, cmdList);
		cmdList->Close();

		ResourceManager::Get().CreateFence(0U, D3D12_FENCE_FLAG_NONE, fence);
	}
}

void SkyBoxPass::Init(
	CommandListExecutor& cmdListExecutor,
	ID3D12CommandQueue& cmdQueue,
	ID3D12Resource& skyBoxCubeMap,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept {

	ASSERT(IsDataValid() == false);

	CreateCommandObjects(mCmdAlloc, mCmdList, mFence);
	mCmdListExecutor = &cmdListExecutor;

	CHECK_HR(mCmdList->Reset(mCmdAlloc, nullptr));

	// Create sky box sphere
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().CreateSphere(3000, 50, 50, model, *mCmdList, uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);
	const std::vector<Mesh>& meshes(model->Meshes());
	ASSERT(meshes.size() == 1UL);

	// Build world matrix
	const Mesh& mesh{ meshes[0] };
	DirectX::XMFLOAT4X4 w;
	MathUtils::ComputeMatrix(w, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	
	DXUtils::ExecuteCommandListAndWaitForCompletion(cmdQueue, *mCmdList, *mFence);

	// Initialize recoders's pso
	SkyBoxCmdListRecorder::InitPSO();

	// Initialize recorder
	mRecorder.reset(new SkyBoxCmdListRecorder(cmdListExecutor.CmdListQueue()));
	mRecorder->Init(
		mesh.VertexBufferData(),
		mesh.IndexBufferData(), 
		w, 
		skyBoxCubeMap,
		colorBufferCpuDesc,
		depthBufferCpuDesc);

	ASSERT(IsDataValid());
}

void SkyBoxPass::Execute(const FrameCBuffer& frameCBuffer) const noexcept {
	ASSERT(IsDataValid());

	mCmdListExecutor->ResetExecutedCmdListCount();
	mRecorder->RecordAndPushCommandLists(frameCBuffer);

	// Wait until all previous tasks command lists are executed
	while (mCmdListExecutor->ExecutedCmdListCount() < 1U) {
		Sleep(0U);
	}
}

bool SkyBoxPass::IsDataValid() const noexcept {
	const bool b =
		mCmdListExecutor != nullptr &&
		mRecorder.get() != nullptr &&
		mCmdAlloc != nullptr &&
		mCmdList != nullptr &&
		mFence != nullptr;

	return b;
}