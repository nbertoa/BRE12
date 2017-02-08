#include "BlurCmdListRecorder.h"

#include <d3d12.h>
#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <PSOManager/PSOManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils/DebugUtils.h>

// Root Signature:
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL)" 0 -> Color Buffer Texture

namespace {
	ID3D12PipelineState* sPSO{ nullptr };
	ID3D12RootSignature* sRootSignature{ nullptr };
}

void BlurCmdListRecorder::InitSharedPSOAndRootSignature() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSignature == nullptr);

	// Build pso and root signature
	PSOManager::PSOCreationData psoData{};
	const std::size_t renderTargetCount{ _countof(psoData.mRenderTargetFormats) };
	psoData.mDepthStencilDescriptor = D3DFactory::GetDisabledDepthStencilDesc();

	psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("AmbientLightPass/Shaders/Blur/PS.cso");
	psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("AmbientLightPass/Shaders/Blur/VS.cso");

	ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("AmbientLightPass/Shaders/Blur/RS.cso");
	psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
	sRootSignature = psoData.mRootSignature;

	psoData.mNumRenderTargets = 1U;
	psoData.mRenderTargetFormats[0U] = DXGI_FORMAT_R16_UNORM;
	for (std::size_t i = psoData.mNumRenderTargets; i < renderTargetCount; ++i) {
		psoData.mRenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	sPSO = &PSOManager::CreateGraphicsPSO(psoData);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
}

void BlurCmdListRecorder::Init(
	ID3D12Resource& inputColorBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc) noexcept
{
	ASSERT(ValidateData() == false);

	mOutputColorBufferCpuDesc = outputColorBufferCpuDesc;

	InitShaderResourceViews(inputColorBuffer);

	ASSERT(ValidateData());
}

void BlurCmdListRecorder::RecordAndPushCommandLists() noexcept {
	ASSERT(ValidateData());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
	
	ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetWithNextCommandAllocator(sPSO);

	commandList.RSSetViewports(1U, &SettingsManager::sScreenViewport);
	commandList.RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	commandList.OMSetRenderTargets(1U, &mOutputColorBufferCpuDesc, false, nullptr);

	ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
	commandList.SetDescriptorHeaps(_countof(heaps), heaps);
	commandList.SetGraphicsRootSignature(sRootSignature);

	// Set root parameters
	commandList.SetGraphicsRootDescriptorTable(0U, mInputColorBufferGpuDesc);

	// Draw object
	commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.DrawInstanced(6U, 1U, 0U, 0U);

	commandList.Close();

	CommandListExecutor::Get().AddCommandList(commandList);
}

bool BlurCmdListRecorder::ValidateData() const noexcept {
	const bool result =
		mInputColorBufferGpuDesc.ptr != 0UL && 
		mOutputColorBufferCpuDesc.ptr != 0UL;

	return result;
}

void BlurCmdListRecorder::InitShaderResourceViews(ID3D12Resource& inputColorBuffer) noexcept {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
	srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDescriptor.Texture2D.MostDetailedMip = 0;
	srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDescriptor.Format = inputColorBuffer.GetDesc().Format;
	srvDescriptor.Texture2D.MipLevels = inputColorBuffer.GetDesc().MipLevels;
	mInputColorBufferGpuDesc = CbvSrvUavDescriptorManager::CreateShaderResourceView(inputColorBuffer, srvDescriptor);
}