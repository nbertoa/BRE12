#include "SkyBoxPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandManager.h>
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
		CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc);
		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc, cmdList);
		cmdList->Close();

		ResourceManager::Get().CreateFence(0U, D3D12_FENCE_FLAG_NONE, fence);
	}

	void ExecuteCommandList(
		ID3D12CommandQueue& cmdQueue,
		ID3D12GraphicsCommandList& cmdList,
		ID3D12Fence& fence) noexcept {

		cmdList.Close();

		ID3D12CommandList* cmdLists[1U]{ &cmdList };
		cmdQueue.ExecuteCommandLists(_countof(cmdLists), cmdLists);

		const std::uint64_t fenceValue = fence.GetCompletedValue() + 1UL;

		CHECK_HR(cmdQueue.Signal(&fence, fenceValue));

		// Wait until the GPU has completed commands up to this fence point.
		if (fence.GetCompletedValue() < fenceValue) {
			const HANDLE eventHandle{ CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS) };
			ASSERT(eventHandle);

			// Fire event when GPU hits current fence.  
			CHECK_HR(fence.SetEventOnCompletion(fenceValue, eventHandle));

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}
}

void SkyBoxPass::Init(
	ID3D12Device& device,
	CommandListExecutor& cmdListExecutor,
	ID3D12CommandQueue& cmdQueue,
	ID3D12Resource& skyBoxCubeMap,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept {

	ASSERT(ValidateData() == false);

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
	
	ExecuteCommandList(cmdQueue, *mCmdList, *mFence);

	// Initialize recoders's pso
	SkyBoxCmdListRecorder::InitPSO();

	// Initialize recorder
	mRecorder.reset(new SkyBoxCmdListRecorder(device, cmdListExecutor.CmdListQueue()));
	mRecorder->Init(
		mesh.VertexBufferData(),
		mesh.IndexBufferData(), 
		w, 
		skyBoxCubeMap,
		colorBufferCpuDesc,
		depthBufferCpuDesc);

	ASSERT(ValidateData());
}

void SkyBoxPass::Execute(const FrameCBuffer& /*frameCBuffer*/) const noexcept {
	ASSERT(ValidateData());

	mCmdListExecutor->ResetExecutedCmdListCount();
	/*mRecorder->RecordAndPushCommandLists(frameCBuffer);

	// Wait until all previous tasks command lists are executed
	while (mCmdListExecutor->ExecutedCmdListCount() < 1U) {
		Sleep(0U);
	}*/
}

bool SkyBoxPass::ValidateData() const noexcept {
	const bool b =
		mCmdListExecutor != nullptr &&
		mRecorder.get() != nullptr &&
		mCmdAlloc != nullptr &&
		mCmdList != nullptr &&
		mFence != nullptr;

	return b;
}