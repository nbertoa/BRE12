#include "PunctualLightCmdListRecorder.h"

#include <DirectXMath.h>

#include <DescriptorManager\DescriptorManager.h>
#include <LightingPass/PunctualLight.h>
#include <MathUtils/MathUtils.h>
#include <PSOCreator/PSOCreator.h>
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
	ID3D12RootSignature* sRootSign{ nullptr };
}

PunctualLightCmdListRecorder::PunctualLightCmdListRecorder(ID3D12Device& device)
	: LightingPassCmdListRecorder(device)
{
}

void PunctualLightCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSign == nullptr);

	// Build pso and root signature
	PSOCreator::PSOParams psoParams{};
	const std::size_t rtCount{ _countof(psoParams.mRtFormats) };
	psoParams.mBlendDesc = D3DFactory::AlwaysBlendDesc();
	psoParams.mDepthStencilDesc = D3DFactory::DisableDepthStencilDesc();
	psoParams.mGSFilename = "LightingPass/Shaders/PunctualLight/GS.cso";
	psoParams.mPSFilename = "LightingPass/Shaders/PunctualLight/PS.cso";
	psoParams.mRootSignFilename = "LightingPass/Shaders/PunctualLight/RS.cso";
	psoParams.mVSFilename = "LightingPass/Shaders/PunctualLight/VS.cso";
	psoParams.mNumRenderTargets = 1U;
	psoParams.mRtFormats[0U] = Settings::sColorBufferFormat;
	for (std::size_t i = psoParams.mNumRenderTargets; i < rtCount; ++i) {
		psoParams.mRtFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoParams.mTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	PSOCreator::CreatePSO(psoParams, sPSO, sRootSign);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSign != nullptr);
}

void PunctualLightCmdListRecorder::Init(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	const void* lights,
	const std::uint32_t numLights) noexcept
{
	ASSERT(ValidateData() == false);
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
	srvDescVec[resIndex].Format = Settings::sDepthStencilSRVFormat;
	srvDescVec[resIndex].Texture2D.MipLevels = depthBuffer.GetDesc().MipLevels;
	res[resIndex] = &depthBuffer;
	
	// Create textures SRV descriptors
	mTexturesGpuDescHandle = DescriptorManager::Get().CreateShaderResourceView(res.data(), srvDescVec.data(), static_cast<uint32_t>(srvDescVec.size()));

	// Create lights buffer SRV	descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mLightsBuffer->Resource()->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0UL;
	srvDesc.Buffer.NumElements = mNumLights;
	srvDesc.Buffer.StructureByteStride = sizeof(PunctualLight);
	mLightsBufferGpuDescHandleBegin = DescriptorManager::Get().CreateShaderResourceView(*mLightsBuffer->Resource(), srvDesc);
	
	ASSERT(ValidateData());
}

void PunctualLightCmdListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(ValidateData());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSign != nullptr);
	ASSERT(mCmdListQueue != nullptr);
	ASSERT(mColorBufferCpuDesc.ptr != 0UL);
	ASSERT(mDepthBufferCpuDesc.ptr != 0UL);

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[mCurrFrameIndex] };
	ASSERT(cmdAlloc != nullptr);	

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(*mFrameCBuffer[mCurrFrameIndex]);
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, sPSO));

	mCmdList->RSSetViewports(1U, &Settings::sScreenViewport);
	mCmdList->RSSetScissorRects(1U, &Settings::sScissorRect);
	mCmdList->OMSetRenderTargets(1U, &mColorBufferCpuDesc, false, &mDepthBufferCpuDesc);

	ID3D12DescriptorHeap* heaps[] = { &DescriptorManager::Get().GetCbvSrcUavDescriptorHeap() };
	mCmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	mCmdList->SetGraphicsRootSignature(sRootSign);

	// Set root parameters
	const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.Resource()->GetGPUVirtualAddress());
	const D3D12_GPU_VIRTUAL_ADDRESS immutableCBufferGpuVAddress(mImmutableCBuffer->Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootDescriptorTable(1U, mLightsBufferGpuDescHandleBegin);
	mCmdList->SetGraphicsRootConstantBufferView(2U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootConstantBufferView(3U, immutableCBufferGpuVAddress);
	mCmdList->SetGraphicsRootConstantBufferView(4U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootDescriptorTable(5U, mTexturesGpuDescHandle);
	
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	
	mCmdList->DrawInstanced(mNumLights, 1U, 0U, 0U);

	mCmdList->Close();

	mCmdListQueue->push(mCmdList);

	// Next frame
	mCurrFrameIndex = (mCurrFrameIndex + 1) % _countof(mCmdAlloc);
}

bool PunctualLightCmdListRecorder::ValidateData() const noexcept {
	return LightingPassCmdListRecorder::ValidateData() && mTexturesGpuDescHandle.ptr != 0UL;
}

void PunctualLightCmdListRecorder::BuildBuffers(const void* lights) noexcept {
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
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
	const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}

	// Create immutable cbuffer
	const std::size_t immutableCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(ImmutableCBuffer)) };
	ResourceManager::Get().CreateUploadBuffer(immutableCBufferElemSize, 1U, mImmutableCBuffer);
	ImmutableCBuffer immutableCBuffer;
	mImmutableCBuffer->CopyData(0U, &immutableCBuffer, sizeof(immutableCBuffer));
}