#include "SkyBoxPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\Model.h>
#include <ModelManager\ModelManager.h>
#include <SkyBoxPass\SkyBoxCmdListRecorder.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateCommandObjects(
		ID3D12CommandAllocator* &commandAllocator,
		ID3D12GraphicsCommandList* &commandList) noexcept 
	{
		// Create command allocators and command list
		commandAllocator = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
		commandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocator);
		commandList->Close();
	}
}

void SkyBoxPass::Init(
	ID3D12Resource& skyBoxCubeMap,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept 
{
	ASSERT(IsDataValid() == false);

	ID3D12CommandAllocator* commandAllocator;
	ID3D12GraphicsCommandList* commandList;
	CreateCommandObjects(commandAllocator, commandList);

	CHECK_HR(commandList->Reset(commandAllocator, nullptr));

	// Create sky box sphere
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	model = &ModelManager::CreateSphere(3000, 50, 50, *commandList, uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);
	const std::vector<Mesh>& meshes(model->GetMeshes());
	ASSERT(meshes.size() == 1UL);

	// Build world matrix
	const Mesh& mesh{ meshes[0] };
	DirectX::XMFLOAT4X4 worldMatrix;
	MathUtils::ComputeMatrix(worldMatrix, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	commandList->Close();

	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(*commandList);

	// Initialize recoders's pso
	SkyBoxCmdListRecorder::InitPSO();

	// Initialize recorder
	mCommandListRecorder.reset(new SkyBoxCmdListRecorder());
	mCommandListRecorder->Init(
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
	mCommandListRecorder->RecordAndPushCommandLists(frameCBuffer);
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < 1U) {
		Sleep(0U);
	}
}

bool SkyBoxPass::IsDataValid() const noexcept {
	const bool b = mCommandListRecorder.get() != nullptr;

	return b;
}