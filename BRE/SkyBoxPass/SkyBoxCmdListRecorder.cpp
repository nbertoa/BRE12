#include "SkyBoxCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <CommandManager/CommandAllocatorManager.h>
#include <CommandManager/CommandListManager.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DirectXManager\DirectXManager.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/UploadBufferManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

// Root Signature:
// "DescriptorTable(CBV(b0), visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Object CBuffers
// "CBV(b1, visibility = SHADER_VISIBILITY_VERTEX), " \ 1 > Frame CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \ 2 -> Cube Map texture

namespace {
	ID3D12PipelineState* sPSO{ nullptr };
	ID3D12RootSignature* sRootSignature{ nullptr };

	void BuildCommandObjects(ID3D12GraphicsCommandList* &cmdList, ID3D12CommandAllocator* cmdAlloc[], const std::size_t cmdAllocCount) noexcept {
		ASSERT(cmdList == nullptr);

#ifdef _DEBUG
		for (std::uint32_t i = 0U; i < cmdAllocCount; ++i) {
			ASSERT(cmdAlloc[i] == nullptr);
		}
#endif

		for (std::uint32_t i = 0U; i < cmdAllocCount; ++i) {
			CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc[i]);
		}

		CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc[0], cmdList);

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		cmdList->Close();
	}
}

SkyBoxCmdListRecorder::SkyBoxCmdListRecorder() {
	BuildCommandObjects(mCommandList, mCommandAllocators, _countof(mCommandAllocators));
}

void SkyBoxCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSignature == nullptr);

	// Build pso and root signature
	PSOManager::PSOCreationData psoData{};
	const std::size_t renderTargetCount{ _countof(psoData.mRenderTargetFormats) };
	// The camera is inside the sky sphere, so just turn off culling.
	psoData.mRasterizerDescriptor.CullMode = D3D12_CULL_MODE_NONE;
	// Make sure the depth function is LESS_EQUAL and not just GREATER.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	psoData.mDepthStencilDescriptor.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoData.mInputLayoutDescriptors = D3DFactory::GetPosNormalTangentTexCoordInputLayout();

	ShaderManager::LoadShaderFile("SkyBoxPass/Shaders/PS.cso", psoData.mPixelShaderBytecode);
	ShaderManager::LoadShaderFile("SkyBoxPass/Shaders/VS.cso", psoData.mVertexShaderBytecode);

	ID3DBlob* rootSignatureBlob;
	ShaderManager::LoadShaderFile("SkyBoxPass/Shaders/RS.cso", rootSignatureBlob);
	RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob, psoData.mRootSignature);
	sRootSignature = psoData.mRootSignature;

	psoData.mNumRenderTargets = 1U;
	psoData.mRenderTargetFormats[0U] = SettingsManager::sColorBufferFormat;
	for (std::size_t i = psoData.mNumRenderTargets; i < renderTargetCount; ++i) {
		psoData.mRenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSOManager::CreateGraphicsPSO(psoData, sPSO);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
}

void SkyBoxCmdListRecorder::Init(
	const VertexAndIndexBufferCreator::VertexBufferData& vertexBufferData,
	const VertexAndIndexBufferCreator::IndexBufferData indexBufferData, 
	const DirectX::XMFLOAT4X4& worldMatrix,
	ID3D12Resource& skyBoxCubeMap,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept
{
	ASSERT(IsDataValid() == false);

	mVertexBufferData = vertexBufferData;
	mIndexBufferData = indexBufferData;
	mWorldMatrix = worldMatrix;
	mOutputColorBufferCpuDesc = outputColorBufferCpuDesc;
	mDepthBufferCpuDesc = depthBufferCpuDesc;

	BuildBuffers(skyBoxCubeMap);

	ASSERT(IsDataValid());
}

void SkyBoxCmdListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);

	static std::uint32_t currentFrameIndex = 0U;

	ID3D12CommandAllocator* commandAllocator{ mCommandAllocators[currentFrameIndex] };
	ASSERT(commandAllocator != nullptr);

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(*mFrameCBuffer[currentFrameIndex]);
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

	CHECK_HR(commandAllocator->Reset());
	CHECK_HR(mCommandList->Reset(commandAllocator, sPSO));

	mCommandList->RSSetViewports(1U, &SettingsManager::sScreenViewport);
	mCommandList->RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	mCommandList->OMSetRenderTargets(1U, &mOutputColorBufferCpuDesc, false, &mDepthBufferCpuDesc);

	ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
	mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);
	mCommandList->SetGraphicsRootSignature(sRootSignature);

	D3D12_GPU_DESCRIPTOR_HANDLE objectCBufferGpuDesc(mObjectCBufferGpuDescBegin);
	D3D12_GPU_DESCRIPTOR_HANDLE cubeMapBufferGpuDesc(mCubeMapBufferGpuDescBegin);

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set frame constants root parameters
	D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.GetResource()->GetGPUVirtualAddress());
	mCommandList->SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	
	// Draw object
	mCommandList->IASetVertexBuffers(0U, 1U, &mVertexBufferData.mBufferView);
	mCommandList->IASetIndexBuffer(&mIndexBufferData.mBufferView);
	mCommandList->SetGraphicsRootDescriptorTable(0U, objectCBufferGpuDesc);
	mCommandList->SetGraphicsRootDescriptorTable(2U, cubeMapBufferGpuDesc);

	mCommandList->DrawIndexedInstanced(mIndexBufferData.mElementCount, 1U, 0U, 0U, 0U);

	mCommandList->Close();

	CommandListExecutor::Get().AddCommandList(*mCommandList);

	// Next frame
	currentFrameIndex = (currentFrameIndex + 1) % SettingsManager::sQueuedFrameCount;
}

bool SkyBoxCmdListRecorder::IsDataValid() const noexcept {
	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocators[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mFrameCBuffer[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		mCommandList != nullptr &&
		mObjectCBuffer != nullptr &&
		mObjectCBufferGpuDescBegin.ptr != 0UL &&
		mCubeMapBufferGpuDescBegin.ptr != 0UL &&
		mOutputColorBufferCpuDesc.ptr != 0UL &&
		mDepthBufferCpuDesc.ptr != 0UL;

	return result;
}

void SkyBoxCmdListRecorder::BuildBuffers(ID3D12Resource& skyBoxCubeMap) noexcept {
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		ASSERT(mFrameCBuffer[i] == nullptr);
	}
#endif
	ASSERT(mObjectCBuffer == nullptr);

	// Create object cbuffer and fill it
	const std::size_t objCBufferElemSize{ UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(ObjectCBuffer)) };
	UploadBufferManager::CreateUploadBuffer(objCBufferElemSize, 1U, mObjectCBuffer);
	ObjectCBuffer objCBuffer;
	const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mWorldMatrix));
	DirectX::XMStoreFloat4x4(&objCBuffer.mWorldMatrix, wMatrix);
	mObjectCBuffer->CopyData(0U, &objCBuffer, sizeof(objCBuffer));
	
	// Set begin for cube map in GPU
	const std::size_t descHandleIncSize{ DirectXManager::GetDevice().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	mCubeMapBufferGpuDescBegin.ptr = mObjectCBufferGpuDescBegin.ptr + descHandleIncSize;

	// Create object cbuffer descriptor
	D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferDesc{};
	const D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ mObjectCBuffer->GetResource()->GetGPUVirtualAddress() };
	cBufferDesc.BufferLocation = objCBufferGpuAddress;
	cBufferDesc.SizeInBytes = static_cast<std::uint32_t>(objCBufferElemSize);
	mObjectCBufferGpuDescBegin = CbvSrvUavDescriptorManager::CreateConstantBufferView(cBufferDesc);

	// Create cube map texture descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = skyBoxCubeMap.GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = skyBoxCubeMap.GetDesc().Format;
	mCubeMapBufferGpuDescBegin = CbvSrvUavDescriptorManager::CreateShaderResourceView(skyBoxCubeMap, srvDesc);

	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		UploadBufferManager::CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}
}