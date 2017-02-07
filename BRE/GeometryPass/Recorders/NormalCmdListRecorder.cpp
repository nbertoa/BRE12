#include "NormalCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DirectXManager\DirectXManager.h>
#include <MaterialManager/Material.h>
#include <MathUtils/MathUtils.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/UploadBufferManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

// Root Signature:
// "DescriptorTable(CBV(b0), visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Object CBuffers
// "CBV(b1, visibility = SHADER_VISIBILITY_VERTEX), " \ 1 -> Frame CBuffers
// "DescriptorTable(CBV(b0), visibility = SHADER_VISIBILITY_PIXEL), " \ 2 -> Material CBuffers
// "CBV(b1, visibility = SHADER_VISIBILITY_PIXEL), " \ 3 -> Frame CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \ 4 -> Diffuse Texture
// "DescriptorTable(SRV(t1), visibility = SHADER_VISIBILITY_PIXEL), " \ 5 -> Normal Texture

namespace {
	ID3D12PipelineState* sPSO{ nullptr };
	ID3D12RootSignature* sRootSignature{ nullptr };
}

void NormalCmdListRecorder::InitPSO(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept {
	ASSERT(geometryBufferFormats != nullptr);
	ASSERT(geometryBufferCount > 0U);
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSignature == nullptr);

	// Build pso and root signature
	PSOManager::PSOCreationData psoData{};
	psoData.mInputLayoutDescriptors = D3DFactory::GetPosNormalTangentTexCoordInputLayout();
	psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/NormalMapping/PS.cso");
	psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/NormalMapping/VS.cso");

	ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("GeometryPass/Shaders/NormalMapping/RS.cso");
	psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
	sRootSignature = psoData.mRootSignature;

	psoData.mNumRenderTargets = geometryBufferCount;
	memcpy(psoData.mRenderTargetFormats, geometryBufferFormats, sizeof(DXGI_FORMAT) * psoData.mNumRenderTargets);
	sPSO = &PSOManager::CreateGraphicsPSO(psoData);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
}

void NormalCmdListRecorder::Init(
	const GeometryData* geometryDataVec,
	const std::uint32_t geometryDataCount,
	const Material* materials,
	ID3D12Resource** textures,
	ID3D12Resource** normals,
	const std::uint32_t numResources) noexcept
{
	ASSERT(IsDataValid() == false);
	ASSERT(geometryDataVec != nullptr);
	ASSERT(geometryDataCount != 0U);
	ASSERT(materials != nullptr);	
	ASSERT(textures != nullptr);
	ASSERT(normals != nullptr);
	ASSERT(numResources > 0UL);

	// Check that the total number of matrices (geometry to be drawn) will be equal to available materials
#ifdef _DEBUG
	std::size_t totalNumMatrices{ 0UL };
	for (std::size_t i = 0UL; i < geometryDataCount; ++i) {
		const std::size_t numMatrices{ geometryDataVec[i].mWorldMatrices.size() };
		totalNumMatrices += numMatrices;
		ASSERT(numMatrices != 0UL);
	}
	ASSERT(totalNumMatrices == numResources);
#endif
	mGeometryDataVec.reserve(geometryDataCount);
	for (std::uint32_t i = 0U; i < geometryDataCount; ++i) {
		mGeometryDataVec.push_back(geometryDataVec[i]);
	}

	InitConstantBuffers(materials, textures, normals, numResources);

	ASSERT(IsDataValid());
}

void NormalCmdListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
	ASSERT(mGeometryBuffersCpuDescs != nullptr);
	ASSERT(mGeometryBuffersCpuDescCount != 0U);
	ASSERT(mDepthBufferCpuDesc.ptr != 0U);
	
	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(*mFrameCBuffer[mCurrentFrameIndex]);
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

	ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetWithNextCommandAllocator(sPSO);

	commandList.RSSetViewports(1U, &SettingsManager::sScreenViewport);
	commandList.RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	commandList.OMSetRenderTargets(mGeometryBuffersCpuDescCount, mGeometryBuffersCpuDescs, false, &mDepthBufferCpuDesc);

	ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
	commandList.SetDescriptorHeaps(_countof(heaps), heaps);
	commandList.SetGraphicsRootSignature(sRootSignature);

	const std::size_t descHandleIncSize{ DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	D3D12_GPU_DESCRIPTOR_HANDLE objectCBufferGpuDesc(mObjectCBufferGpuDescBegin);
	D3D12_GPU_DESCRIPTOR_HANDLE materialsCBufferGpuDesc(mMaterialsCBufferGpuDescBegin);
	D3D12_GPU_DESCRIPTOR_HANDLE texturesBufferGpuDesc(mBaseColorBufferGpuDescBegin);
	D3D12_GPU_DESCRIPTOR_HANDLE normalsBufferGpuDesc(mNormalsBufferGpuDescBegin);

	commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set frame constants root parameters
	D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.GetResource()->GetGPUVirtualAddress());
	commandList.SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	commandList.SetGraphicsRootConstantBufferView(3U, frameCBufferGpuVAddress);
	
	// Draw objects
	const std::size_t geomCount{ mGeometryDataVec.size() };
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		GeometryData& geomData{ mGeometryDataVec[i] };
		commandList.IASetVertexBuffers(0U, 1U, &geomData.mVertexBufferData.mBufferView);
		commandList.IASetIndexBuffer(&geomData.mIndexBufferData.mBufferView);
		const std::size_t worldMatsCount{ geomData.mWorldMatrices.size() };
		for (std::size_t j = 0UL; j < worldMatsCount; ++j) {
			commandList.SetGraphicsRootDescriptorTable(0U, objectCBufferGpuDesc);
			objectCBufferGpuDesc.ptr += descHandleIncSize;

			commandList.SetGraphicsRootDescriptorTable(2U, materialsCBufferGpuDesc);
			materialsCBufferGpuDesc.ptr += descHandleIncSize;

			commandList.SetGraphicsRootDescriptorTable(4U, texturesBufferGpuDesc);
			texturesBufferGpuDesc.ptr += descHandleIncSize;

			commandList.SetGraphicsRootDescriptorTable(5U, normalsBufferGpuDesc);
			normalsBufferGpuDesc.ptr += descHandleIncSize;

			commandList.DrawIndexedInstanced(geomData.mIndexBufferData.mElementCount, 1U, 0U, 0U, 0U);
		}
	}

	commandList.Close();

	CommandListExecutor::Get().AddCommandList(commandList);

	// Next frame
	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % SettingsManager::sQueuedFrameCount;
}

bool NormalCmdListRecorder::IsDataValid() const noexcept {
	const bool result =
		GeometryPassCmdListRecorder::IsDataValid() &&
		mBaseColorBufferGpuDescBegin.ptr != 0UL && 
		mNormalsBufferGpuDescBegin.ptr != 0UL;

	return result;
}

void NormalCmdListRecorder::InitConstantBuffers(
	const Material* materials, 
	ID3D12Resource** textures, 
	ID3D12Resource** normals,
	const std::uint32_t dataCount) noexcept 
{
	ASSERT(materials != nullptr);
	ASSERT(textures != nullptr);
	ASSERT(normals != nullptr);
	ASSERT(dataCount != 0UL);
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		ASSERT(mFrameCBuffer[i] == nullptr);
	}
#endif
	ASSERT(mObjectCBuffer == nullptr);
	ASSERT(mMaterialsCBuffer == nullptr);

	// Create object cbuffer and fill it
	const std::size_t objCBufferElemSize{ UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(ObjectCBuffer)) };
	mObjectCBuffer = &UploadBufferManager::CreateUploadBuffer(objCBufferElemSize, dataCount);
	std::uint32_t k = 0U;
	const std::size_t geometryDataCount{ mGeometryDataVec.size() };
	ObjectCBuffer objCBuffer;
	for (std::size_t i = 0UL; i < geometryDataCount; ++i) {
		GeometryData& geomData{ mGeometryDataVec[i] };
		const std::uint32_t worldMatsCount{ static_cast<std::uint32_t>(geomData.mWorldMatrices.size()) };
		for (std::uint32_t j = 0UL; j < worldMatsCount; ++j) {
			const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&geomData.mWorldMatrices[j]));
			DirectX::XMStoreFloat4x4(&objCBuffer.mWorldMatrix, wMatrix);
			mObjectCBuffer->CopyData(k + j, &objCBuffer, sizeof(objCBuffer));
		}

		k += worldMatsCount;
	}

	// Create materials cbuffer		
	const std::size_t matCBufferElemSize{ UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(Material)) };
	mMaterialsCBuffer = &UploadBufferManager::CreateUploadBuffer(matCBufferElemSize, dataCount);

	D3D12_GPU_VIRTUAL_ADDRESS materialsGpuAddress{ mMaterialsCBuffer->GetResource()->GetGPUVirtualAddress() };
	D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ mObjectCBuffer->GetResource()->GetGPUVirtualAddress() };

	// Create object / materials cbuffers descriptors
	// Create textures SRV descriptors
	std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> objectCbufferViewDescVec;
	objectCbufferViewDescVec.reserve(dataCount);
	
	std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> materialCbufferViewDescVec;
	materialCbufferViewDescVec.reserve(dataCount);
	
	std::vector<ID3D12Resource*> textureResVec;
	textureResVec.reserve(dataCount);
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> textureSrvDescVec;
	textureSrvDescVec.reserve(dataCount);

	std::vector<ID3D12Resource*> normalResVec;
	normalResVec.reserve(dataCount);
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> normalSrvDescVec;
	normalSrvDescVec.reserve(dataCount);
	for (std::size_t i = 0UL; i < dataCount; ++i) {
		// Object cbuffer desc
		D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferDesc{};
		cBufferDesc.BufferLocation = objCBufferGpuAddress + i * objCBufferElemSize;
		cBufferDesc.SizeInBytes = static_cast<std::uint32_t>(objCBufferElemSize);
		objectCbufferViewDescVec.push_back(cBufferDesc);

		// Material cbuffer desc
		cBufferDesc.BufferLocation = materialsGpuAddress + i * matCBufferElemSize;
		cBufferDesc.SizeInBytes = static_cast<std::uint32_t>(matCBufferElemSize);
		materialCbufferViewDescVec.push_back(cBufferDesc);

		// Texture descriptor
		textureResVec.push_back(textures[i]);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Format = textureResVec.back()->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = textureResVec.back()->GetDesc().MipLevels;
		textureSrvDescVec.push_back(srvDesc);

		// Normal descriptor
		normalResVec.push_back(normals[i]);

		srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Format = normalResVec.back()->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = normalResVec.back()->GetDesc().MipLevels;
		normalSrvDescVec.push_back(srvDesc);

		mMaterialsCBuffer->CopyData(static_cast<std::uint32_t>(i), &materials[i], sizeof(Material));
	}
	mObjectCBufferGpuDescBegin =
		CbvSrvUavDescriptorManager::CreateConstantBufferViews(
			objectCbufferViewDescVec.data(), 
			static_cast<std::uint32_t>(objectCbufferViewDescVec.size()));
	mMaterialsCBufferGpuDescBegin =
		CbvSrvUavDescriptorManager::CreateConstantBufferViews(
			materialCbufferViewDescVec.data(), 
			static_cast<std::uint32_t>(materialCbufferViewDescVec.size()));
	mBaseColorBufferGpuDescBegin =
		CbvSrvUavDescriptorManager::CreateShaderResourceViews(
			textureResVec.data(), 
			textureSrvDescVec.data(), 
			static_cast<std::uint32_t>(textureSrvDescVec.size()));
	mNormalsBufferGpuDescBegin =
		CbvSrvUavDescriptorManager::CreateShaderResourceViews(
			normalResVec.data(), 
			normalSrvDescVec.data(), 
			static_cast<std::uint32_t>(normalSrvDescVec.size()));
	
	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		mFrameCBuffer[i] = &UploadBufferManager::CreateUploadBuffer(frameCBufferElemSize, 1U);
	}
}