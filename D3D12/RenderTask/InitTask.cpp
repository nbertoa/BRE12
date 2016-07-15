#include "InitTask.h"

#include <CommandManager/CommandManager.h>
#include <GlobalData\Settings.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager/RootSignatureManager.h>
#include <ShaderManager/ShaderManager.h>
#include <Utils/DebugUtils.h>

bool InitTaskInput::ValidateData() const {
	return mVertexIndexBufferCreatorInputVec.empty() == false && mPSOCreatorInput.ValidateData();
}

void InitTask::InitCmdBuilders(ID3D12Device& /*device*/, tbb::concurrent_queue<ID3D12CommandList*>& /*cmdLists*/, CmdBuilderTaskInput& output) noexcept {
	ASSERT(mInput.ValidateData());
	
	output.mTopology = mInput.mPSOCreatorInput.mTopology;

	BuildPSO(output.mRootSign, output.mPSO);
	BuildCommandObjects(output);
}

void InitTask::BuildPSO(ID3D12RootSignature* &rootSign, ID3D12PipelineState* &pso) noexcept {
	std::vector<PSOCreatorTask::Output> outputs;
	PSOCreatorTask task({mInput.mPSOCreatorInput});
	task.Execute(outputs);

	ASSERT(outputs.size() == 1UL);
	rootSign = outputs[0].mRootSign;
	pso = outputs[0].mPSO;
}

void InitTask::BuildVertexAndIndexBuffers(
	GeometryData& geomData,
	const VertexIndexBufferCreatorTask::Input& vertexIndexBuffers,
	ID3D12GraphicsCommandList& cmdList) noexcept {

	ASSERT(geomData.mBuffersInfo.mIndexBuffer == nullptr);
	ASSERT(geomData.mBuffersInfo.mVertexBuffer == nullptr);
	ASSERT(vertexIndexBuffers.mVertsData != nullptr);
	ASSERT(vertexIndexBuffers.mNumVerts > 0U);
	ASSERT(vertexIndexBuffers.mVertexSize > 0UL);
	ASSERT(vertexIndexBuffers.mIndexData != nullptr);
	ASSERT(vertexIndexBuffers.mNumIndices > 0U);

	std::uint32_t byteSize{ vertexIndexBuffers.mNumVerts * (std::uint32_t)vertexIndexBuffers.mVertexSize };

	ResourceManager::gManager->CreateDefaultBuffer(cmdList, vertexIndexBuffers.mVertsData, byteSize, geomData.mBuffersInfo.mVertexBuffer, geomData.mBuffersInfo.mUploadVertexBuffer);
	geomData.mBuffersInfo.mVertexBufferView.BufferLocation = geomData.mBuffersInfo.mVertexBuffer->GetGPUVirtualAddress();
	geomData.mBuffersInfo.mVertexBufferView.SizeInBytes = byteSize;
	geomData.mBuffersInfo.mVertexBufferView.StrideInBytes = (std::uint32_t)vertexIndexBuffers.mVertexSize;

	geomData.mBuffersInfo.mIndexCount = vertexIndexBuffers.mNumIndices;
	byteSize = vertexIndexBuffers.mNumIndices * sizeof(std::uint32_t);
	ResourceManager::gManager->CreateDefaultBuffer(cmdList, vertexIndexBuffers.mIndexData, byteSize, geomData.mBuffersInfo.mIndexBuffer, geomData.mBuffersInfo.mUploadIndexBuffer);
	geomData.mBuffersInfo.mIndexBufferView.BufferLocation = geomData.mBuffersInfo.mIndexBuffer->GetGPUVirtualAddress();
	geomData.mBuffersInfo.mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	geomData.mBuffersInfo.mIndexBufferView.SizeInBytes = byteSize;
}

void InitTask::BuildCommandObjects(CmdBuilderTaskInput& output) noexcept {
	ASSERT(output.mCmdList == nullptr);
	const std::uint32_t allocCount{ _countof(output.mCmdAlloc) };

#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < allocCount; ++i) {
		ASSERT(output.mCmdAlloc[i] == nullptr);
	}	
#endif
	
	for (std::uint32_t i = 0U; i < allocCount; ++i) {
		CommandManager::gManager->CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, output.mCmdAlloc[i]);
	}

	CommandManager::gManager->CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *output.mCmdAlloc[0], output.mCmdList);

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	output.mCmdList->Close();
}