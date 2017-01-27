#include "PunctualLightCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <LightingPass/PunctualLight.h>
#include <MathUtils/MathUtils.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

// Root Signature:
// "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Frame CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_VERTEX), " \ 1 -> Lights Buffer
// "CBV(b0, visibility = SHADER_VISIBILITY_GEOMETRY), " \ 2 -> Frame CBuffer
// "CBV(b1, visibility = SHADER_VISIBILITY_GEOMETRY), " \ 3 -> Immutable CBuffer
// "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \ 4 -> Frame CBuffer
// "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), visibility = SHADER_VISIBILITY_PIXEL)" 5 -> Textures

namespace {
	ID3D12PipelineState* sPSO{ nullptr };
	ID3D12RootSignature* sRootSignature{ nullptr };
}

void PunctualLightCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSignature == nullptr);

	// Build pso and root signature
	PSOManager::PSOCreationData psoData{};
	const std::size_t rtCount{ _countof(psoData.mRtFormats) };
	psoData.mBlendDesc = D3DFactory::GetAlwaysBlendDesc();
	psoData.mDepthStencilDesc = D3DFactory::GetDisabledDepthStencilDesc();
	psoData.mGSFilename = "LightingPass/Shaders/PunctualLight/GS.cso";
	psoData.mPSFilename = "LightingPass/Shaders/PunctualLight/PS.cso";
	psoData.mRootSignFilename = "LightingPass/Shaders/PunctualLight/RS.cso";
	psoData.mVSFilename = "LightingPass/Shaders/PunctualLight/VS.cso";
	psoData.mNumRenderTargets = 1U;
	psoData.mRtFormats[0U] = SettingsManager::sColorBufferFormat;
	for (std::size_t i = psoData.mNumRenderTargets; i < rtCount; ++i) {
		psoData.mRtFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoData.mTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	PSOManager::Get().CreateGraphicsPSO(psoData, sPSO, sRootSignature);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
}

void PunctualLightCmdListRecorder::Init(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	const void* lights,
	const std::uint32_t numLights) noexcept
{
	ASSERT(IsDataValid() == false);
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	ASSERT(lights != nullptr);
	ASSERT(numLights > 0U);
	
	mNumLights = numLights;

	BuildBuffers(lights);

	
	// Used to create SRV descriptors for textures (geometry buffers + depth buffer)
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDescVec;
	srvDescVec.resize(geometryBuffersCount + 1U); // 1 = depth buffer
	std::vector<ID3D12Resource*> res;
	res.resize(geometryBuffersCount + 1U);

	// Fill data for geometry buffers SRV descriptors
	for (std::uint32_t i = 0U; i < geometryBuffersCount; ++i) {
		res[i] = geometryBuffers[i].Get();

		srvDescVec[i].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDescVec[i].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDescVec[i].Texture2D.MostDetailedMip = 0;
		srvDescVec[i].Texture2D.ResourceMinLODClamp = 0.0f;
		srvDescVec[i].Format = res[i]->GetDesc().Format;
		srvDescVec[i].Texture2D.MipLevels = res[i]->GetDesc().MipLevels;
	}

	// Fill depth buffer SRV description
	const std::uint32_t resIndex = geometryBuffersCount;
	srvDescVec[resIndex].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescVec[resIndex].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDescVec[resIndex].Texture2D.MostDetailedMip = 0;
	srvDescVec[resIndex].Texture2D.ResourceMinLODClamp = 0.0f;
	srvDescVec[resIndex].Format = SettingsManager::sDepthStencilSRVFormat;
	srvDescVec[resIndex].Texture2D.MipLevels = depthBuffer.GetDesc().MipLevels;
	res[resIndex] = &depthBuffer;
	
	// Create textures SRV descriptors
	mPixelShaderBuffersGpuDesc = CbvSrvUavDescriptorManager::Get().CreateShaderResourceViews(res.data(), srvDescVec.data(), static_cast<uint32_t>(srvDescVec.size()));

	// Create lights buffer SRV	descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mLightsBuffer->GetResource()->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0UL;
	srvDesc.Buffer.NumElements = mNumLights;
	srvDesc.Buffer.StructureByteStride = sizeof(PunctualLight);
	mLightsBufferGpuDescBegin = CbvSrvUavDescriptorManager::Get().CreateShaderResourceView(*mLightsBuffer->GetResource(), srvDesc);
	
	ASSERT(IsDataValid());
}

void PunctualLightCmdListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
	ASSERT(mOutputColorBufferCpuDesc.ptr != 0UL);

	ID3D12CommandAllocator* cmdAlloc{ mCommandAllocators[mCurrentFrameIndex] };
	ASSERT(cmdAlloc != nullptr);	

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(*mFrameCBuffer[mCurrentFrameIndex]);
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCommandList->Reset(cmdAlloc, sPSO));

	mCommandList->RSSetViewports(1U, &SettingsManager::sScreenViewport);
	mCommandList->RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	mCommandList->OMSetRenderTargets(1U, &mOutputColorBufferCpuDesc, false, nullptr);

	ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::Get().GetDescriptorHeap() };
	mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);
	mCommandList->SetGraphicsRootSignature(sRootSignature);

	// Set root parameters
	const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.GetResource()->GetGPUVirtualAddress());
	const D3D12_GPU_VIRTUAL_ADDRESS immutableCBufferGpuVAddress(mImmutableCBuffer->GetResource()->GetGPUVirtualAddress());
	mCommandList->SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
	mCommandList->SetGraphicsRootDescriptorTable(1U, mLightsBufferGpuDescBegin);
	mCommandList->SetGraphicsRootConstantBufferView(2U, frameCBufferGpuVAddress);
	mCommandList->SetGraphicsRootConstantBufferView(3U, immutableCBufferGpuVAddress);
	mCommandList->SetGraphicsRootConstantBufferView(4U, frameCBufferGpuVAddress);
	mCommandList->SetGraphicsRootDescriptorTable(5U, mPixelShaderBuffersGpuDesc);
	
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	
	mCommandList->DrawInstanced(mNumLights, 1U, 0U, 0U);

	mCommandList->Close();

	CommandListExecutor::Get().AddCommandList(*mCommandList);

	// Next frame
	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % _countof(mCommandAllocators);
}

bool PunctualLightCmdListRecorder::IsDataValid() const noexcept {
	return LightingPassCmdListRecorder::IsDataValid() && mPixelShaderBuffersGpuDesc.ptr != 0UL;
}

void PunctualLightCmdListRecorder::BuildBuffers(const void* lights) noexcept {
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		ASSERT(mFrameCBuffer[i] == nullptr);
	}
#endif
	ASSERT(mLightsBuffer == nullptr);
	ASSERT(lights != nullptr);
	ASSERT(mNumLights != 0U);

	// Create lights buffer and fill it
	const std::size_t lightBufferElemSize{ sizeof(PunctualLight) };
	ResourceManager::Get().CreateUploadBuffer(lightBufferElemSize, mNumLights, mLightsBuffer);
	const std::uint8_t* lightsPtr = reinterpret_cast<const std::uint8_t*>(lights);
	for (std::uint32_t i = 0UL; i < mNumLights; ++i) {
		mLightsBuffer->CopyData(i, lightsPtr + sizeof(PunctualLight) * i, sizeof(PunctualLight));
	}

	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::RoundConstantBufferSizeInBytes(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}

	// Create immutable cbuffer
	const std::size_t immutableCBufferElemSize{ UploadBuffer::RoundConstantBufferSizeInBytes(sizeof(ImmutableCBuffer)) };
	ResourceManager::Get().CreateUploadBuffer(immutableCBufferElemSize, 1U, mImmutableCBuffer);
	ImmutableCBuffer immutableCBuffer;
	mImmutableCBuffer->CopyData(0U, &immutableCBuffer, sizeof(immutableCBuffer));
}