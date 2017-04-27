#include "SkyBoxCmdListRecorder.h"

#include <d3d12.h>

#include <CommandListExecutor\CommandListExecutor.h>
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

using namespace DirectX;

namespace {
	ID3D12PipelineState* sPSO{ nullptr };
	ID3D12RootSignature* sRootSignature{ nullptr };
}

void SkyBoxCmdListRecorder::InitSharedPSOAndRootSignature() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSignature == nullptr);

	PSOManager::PSOCreationData psoData{};

	// The camera is inside the sky sphere, so just turn off culling.
	psoData.mRasterizerDescriptor.CullMode = D3D12_CULL_MODE_NONE;

	// Make sure the depth function is LESS_EQUAL and not just GREATER.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	psoData.mDepthStencilDescriptor.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoData.mInputLayoutDescriptors = D3DFactory::GetPosNormalTangentTexCoordInputLayout();

	psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("SkyBoxPass/Shaders/PS.cso");
	psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("SkyBoxPass/Shaders/VS.cso");

	ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("SkyBoxPass/Shaders/RS.cso");
	psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
	sRootSignature = psoData.mRootSignature;

	psoData.mNumRenderTargets = 1U;
	psoData.mRenderTargetFormats[0U] = SettingsManager::sColorBufferFormat;
	for (std::size_t i = psoData.mNumRenderTargets; i < _countof(psoData.mRenderTargetFormats); ++i) {
		psoData.mRenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	sPSO = &PSOManager::CreateGraphicsPSO(psoData);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
}

void SkyBoxCmdListRecorder::Init(
	const VertexAndIndexBufferCreator::VertexBufferData& vertexBufferData,
	const VertexAndIndexBufferCreator::IndexBufferData indexBufferData, 
	const XMFLOAT4X4& worldMatrix,
	ID3D12Resource& skyBoxCubeMap,
	const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept
{
	ASSERT(IsDataValid() == false);

	mVertexBufferData = vertexBufferData;
	mIndexBufferData = indexBufferData;
	mRenderTargetView = renderTargetView;
	mDepthBufferView = depthBufferView;

	InitConstantBuffers(worldMatrix);
	InitShaderResourceViews(skyBoxCubeMap);

	ASSERT(IsDataValid());
}

void SkyBoxCmdListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(mFrameUploadCBufferPerFrame.GetNextFrameCBuffer());
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

	ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetWithNextCommandAllocator(sPSO);

	commandList.RSSetViewports(1U, &SettingsManager::sScreenViewport);
	commandList.RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	commandList.OMSetRenderTargets(1U, &mRenderTargetView, false, &mDepthBufferView);

	ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
	commandList.SetDescriptorHeaps(_countof(heaps), heaps);

	commandList.SetGraphicsRootSignature(sRootSignature);
	D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.GetResource()->GetGPUVirtualAddress());
	commandList.SetGraphicsRootDescriptorTable(0U, mObjectCBufferView);
	commandList.SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	commandList.SetGraphicsRootDescriptorTable(2U, mStartPixelShaderResourceView);

	commandList.IASetVertexBuffers(0U, 1U, &mVertexBufferData.mBufferView);
	commandList.IASetIndexBuffer(&mIndexBufferData.mBufferView);
	commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.DrawIndexedInstanced(mIndexBufferData.mElementCount, 1U, 0U, 0U, 0U);

	commandList.Close();
	CommandListExecutor::Get().AddCommandList(commandList);
}

bool SkyBoxCmdListRecorder::IsDataValid() const noexcept {
	const bool result =
		mObjectUploadCBuffer != nullptr &&
		mObjectCBufferView.ptr != 0UL &&
		mStartPixelShaderResourceView.ptr != 0UL &&
		mRenderTargetView.ptr != 0UL &&
		mDepthBufferView.ptr != 0UL;

	return result;
}

void SkyBoxCmdListRecorder::InitConstantBuffers(const XMFLOAT4X4& worldMatrix) noexcept {
	ASSERT(mObjectUploadCBuffer == nullptr);

	// Create object cbuffer and fill it
	const std::size_t objCBufferElemSize{ UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(ObjectCBuffer)) };
	mObjectUploadCBuffer = &UploadBufferManager::CreateUploadBuffer(objCBufferElemSize, 1U);
	ObjectCBuffer objCBuffer;
	MathUtils::StoreTransposeMatrix(worldMatrix, objCBuffer.mWorldMatrix);
	mObjectUploadCBuffer->CopyData(0U, &objCBuffer, sizeof(objCBuffer));

	// Create object cbufferview
	D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferDesc{};
	const D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ mObjectUploadCBuffer->GetResource()->GetGPUVirtualAddress() };
	cBufferDesc.BufferLocation = objCBufferGpuAddress;
	cBufferDesc.SizeInBytes = static_cast<std::uint32_t>(objCBufferElemSize);
	mObjectCBufferView = CbvSrvUavDescriptorManager::CreateConstantBufferView(cBufferDesc);
}

void SkyBoxCmdListRecorder::InitShaderResourceViews(ID3D12Resource& skyBoxCubeMap) noexcept {	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
	srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDescriptor.TextureCube.MostDetailedMip = 0;
	srvDescriptor.TextureCube.MipLevels = skyBoxCubeMap.GetDesc().MipLevels;
	srvDescriptor.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDescriptor.Format = skyBoxCubeMap.GetDesc().Format;
	mStartPixelShaderResourceView = CbvSrvUavDescriptorManager::CreateShaderResourceView(skyBoxCubeMap, srvDescriptor);
}