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

using namespace DirectX;

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

	Model& CreateAndGetSkyBoxSphereModel(
		ID3D12CommandAllocator& commandAllocator, 
		ID3D12GraphicsCommandList& commandList) 
	{
		CHECK_HR(commandList.Reset(&commandAllocator, nullptr));

		Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
		Model* model = &ModelManager::CreateSphere(3000, 50, 50, commandList, uploadVertexBuffer, uploadIndexBuffer);
		
		commandList.Close();
		CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);

		return *model;
	}
}

void 
SkyBoxPass::Init(
	ID3D12Resource& skyBoxCubeMap,
	const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept 
{
	ASSERT(IsDataValid() == false);

	ID3D12CommandAllocator* commandAllocator;
	ID3D12GraphicsCommandList* commandList;
	CreateCommandObjects(commandAllocator, commandList);

	Model& model = CreateAndGetSkyBoxSphereModel(*commandAllocator, *commandList);
	const std::vector<Mesh>& meshes(model.GetMeshes());
	ASSERT(meshes.size() == 1UL);

	// Build world matrix
	const Mesh& mesh{ meshes[0] };
	XMFLOAT4X4 worldMatrix;
	MathUtils::ComputeMatrix(worldMatrix, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	SkyBoxCmdListRecorder::InitSharedPSOAndRootSignature();

	mCommandListRecorder.reset(new SkyBoxCmdListRecorder());
	mCommandListRecorder->Init(
		mesh.GetVertexBufferData(),
		mesh.GetIndexBufferData(), 
		worldMatrix, 
		skyBoxCubeMap,
		renderTargetView,
		depthBufferView);

	ASSERT(IsDataValid());
}

void 
SkyBoxPass::Execute(const FrameCBuffer& frameCBuffer) const noexcept {
	ASSERT(IsDataValid());

	CommandListExecutor::Get().ResetExecutedCommandListCount();
	mCommandListRecorder->RecordAndPushCommandLists(frameCBuffer);

	// Wait until all previous tasks command lists are executed
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < 1U) {
		Sleep(0U);
	}
}

bool 
SkyBoxPass::IsDataValid() const noexcept {
	const bool b = mCommandListRecorder.get() != nullptr;

	return b;
}