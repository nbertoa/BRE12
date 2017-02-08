#include "EnvironmentLightCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/UploadBufferManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

// Root Signature:
// "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Frame CBuffer
// "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \ 1 -> Frame CBuffer
// "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), visibility = SHADER_VISIBILITY_PIXEL), " \ 2 -> Textures 

namespace {
	ID3D12PipelineState* sPSO{ nullptr };
	ID3D12RootSignature* sRootSignature{ nullptr };
}

void EnvironmentLightCmdListRecorder::InitSharedPSOAndRootSignature() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSignature == nullptr);

	// Build pso and root signature
	PSOManager::PSOCreationData psoData{};
	const std::size_t renderTargetCount{ _countof(psoData.mRenderTargetFormats) };
	psoData.mBlendDescriptor = D3DFactory::GetAlwaysBlendDesc();
	psoData.mDepthStencilDescriptor = D3DFactory::GetDisabledDepthStencilDesc();

	psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("EnvironmentLightPass/Shaders/PS.cso");
	psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("EnvironmentLightPass/Shaders/VS.cso");

	ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("EnvironmentLightPass/Shaders/RS.cso");
	psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
	sRootSignature = psoData.mRootSignature;

	psoData.mNumRenderTargets = 1U;
	psoData.mRenderTargetFormats[0U] = SettingsManager::sColorBufferFormat;
	for (std::size_t i = psoData.mNumRenderTargets; i < renderTargetCount; ++i) {
		psoData.mRenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	sPSO = &PSOManager::CreateGraphicsPSO(psoData);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
}

void EnvironmentLightCmdListRecorder::Init(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc,
	ID3D12Resource& diffuseIrradianceCubeMap,
	ID3D12Resource& specularPreConvolvedCubeMap) noexcept
{
	ASSERT(ValidateData() == false);
	ASSERT(geometryBuffers != nullptr);
	ASSERT(geometryBuffersCount > 0U);

	mOutputColorBufferCpuDesc = outputColorBufferCpuDesc;

	InitShaderResourceViews(geometryBuffers, geometryBuffersCount, depthBuffer, diffuseIrradianceCubeMap, specularPreConvolvedCubeMap);

	ASSERT(ValidateData());
}

void EnvironmentLightCmdListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(ValidateData());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(mFrameCBufferPerFrame.GetNextFrameCBuffer());
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));
	
	ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetWithNextCommandAllocator(sPSO);

	commandList.RSSetViewports(1U, &SettingsManager::sScreenViewport);
	commandList.RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	commandList.OMSetRenderTargets(1U, &mOutputColorBufferCpuDesc, false, nullptr);

	ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
	commandList.SetDescriptorHeaps(_countof(heaps), heaps);
	commandList.SetGraphicsRootSignature(sRootSignature);
	
	// Set root parameters
	const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.GetResource()->GetGPUVirtualAddress());
	commandList.SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
	commandList.SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	commandList.SetGraphicsRootDescriptorTable(2U, mPixelShaderBuffersGpuDesc);

	// Draw object
	commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.DrawInstanced(6U, 1U, 0U, 0U);

	commandList.Close();

	CommandListExecutor::Get().AddCommandList(commandList);
}

bool EnvironmentLightCmdListRecorder::ValidateData() const noexcept {
	const bool result =
		mOutputColorBufferCpuDesc.ptr != 0UL &&
		mPixelShaderBuffersGpuDesc.ptr != 0UL;

	return result;
}

void EnvironmentLightCmdListRecorder::InitShaderResourceViews(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers, 
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	ID3D12Resource& diffuseIrradianceCubeMap,
	ID3D12Resource& specularPreConvolvedCubeMap) noexcept
{
	ASSERT(geometryBuffers != nullptr);
	ASSERT(geometryBuffersCount > 0U);

	// Number of geometry buffers + depth buffer + 2 cube maps
	const std::uint32_t numResources = geometryBuffersCount + 3U;

	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDescriptors;
	srvDescriptors.reserve(numResources);

	std::vector<ID3D12Resource*> resources;
	resources.reserve(numResources);

	// Fill geometry buffers SRV descriptors
	for (std::uint32_t i = 0U; i < geometryBuffersCount; ++i) {
		ASSERT(geometryBuffers[i].Get() != nullptr);

		resources.push_back(geometryBuffers[i].Get());

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
		srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDescriptor.Texture2D.MostDetailedMip = 0;
		srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDescriptor.Format = resources.back()->GetDesc().Format;
		srvDescriptor.Texture2D.MipLevels = resources.back()->GetDesc().MipLevels;

		srvDescriptors.emplace_back(srvDescriptor);
	}

	// Fill depth buffer descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
	srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDescriptor.Texture2D.MostDetailedMip = 0;
	srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDescriptor.Format = SettingsManager::sDepthStencilSRVFormat;
	srvDescriptor.Texture2D.MipLevels = depthBuffer.GetDesc().MipLevels;
	srvDescriptors.emplace_back(srvDescriptor);
	resources.push_back(&depthBuffer);

	// Fill cube map texture descriptors	
	srvDescriptor = D3D12_SHADER_RESOURCE_VIEW_DESC{};
	srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDescriptor.TextureCube.MostDetailedMip = 0;
	srvDescriptor.TextureCube.MipLevels = diffuseIrradianceCubeMap.GetDesc().MipLevels;
	srvDescriptor.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDescriptor.Format = diffuseIrradianceCubeMap.GetDesc().Format;
	srvDescriptors.emplace_back(srvDescriptor);
	resources.push_back(&diffuseIrradianceCubeMap);

	srvDescriptor = D3D12_SHADER_RESOURCE_VIEW_DESC{};
	srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDescriptor.TextureCube.MostDetailedMip = 0;
	srvDescriptor.TextureCube.MipLevels = specularPreConvolvedCubeMap.GetDesc().MipLevels;
	srvDescriptor.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDescriptor.Format = specularPreConvolvedCubeMap.GetDesc().Format;
	srvDescriptors.emplace_back(srvDescriptor);
	resources.push_back(&specularPreConvolvedCubeMap);

	mPixelShaderBuffersGpuDesc = 
		CbvSrvUavDescriptorManager::CreateShaderResourceViews(
			resources.data(), 
			srvDescriptors.data(), 
			numResources);
}