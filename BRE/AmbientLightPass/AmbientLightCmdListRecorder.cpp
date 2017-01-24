#include "AmbientLightCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandManager/CommandManager.h>
#include <DescriptorManager\DescriptorManager.h>
#include <PSOCreator/PSOCreator.h>
#include <Utils/DebugUtils.h>

// Root Signature:
// "DescriptorTable(SRV(t0), SRV(t1), visibility = SHADER_VISIBILITY_PIXEL)" 0 -> BaseColor_MetalMask texture, AmbientAccessibility texture

namespace {
	ID3D12PipelineState* sPSO{ nullptr };
	ID3D12RootSignature* sRootSign{ nullptr };

	void BuildCommandObjects(ID3D12GraphicsCommandList* &cmdList, ID3D12CommandAllocator* cmdAlloc[], const std::size_t cmdAllocCount) noexcept {
		ASSERT(cmdList == nullptr);

#ifdef _DEBUG
		for (std::uint32_t i = 0U; i < cmdAllocCount; ++i) {
			ASSERT(cmdAlloc[i] == nullptr);
		}
#endif

		for (std::uint32_t i = 0U; i < cmdAllocCount; ++i) {
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc[i]);
		}

		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc[0], cmdList);

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		cmdList->Close();
	}
}

AmbientLightCmdListRecorder::AmbientLightCmdListRecorder(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: mCmdListQueue(cmdListQueue)
{
	BuildCommandObjects(mCmdList, mCmdAlloc, _countof(mCmdAlloc));
}

void AmbientLightCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSign == nullptr);

	// Build pso and root signature
	PSOCreator::PSOParams psoParams{};
	const std::size_t rtCount{ _countof(psoParams.mRtFormats) };
	psoParams.mBlendDesc = D3DFactory::GetAlwaysBlendDesc();
	psoParams.mDepthStencilDesc = D3DFactory::GetDisabledDepthStencilDesc();
	psoParams.mPSFilename = "AmbientLightPass/Shaders/AmbientLight/PS.cso";
	psoParams.mRootSignFilename = "AmbientLightPass/Shaders/AmbientLight/RS.cso";
	psoParams.mVSFilename = "AmbientLightPass/Shaders/AmbientLight/VS.cso";
	psoParams.mNumRenderTargets = 1U;
	psoParams.mRtFormats[0U] = SettingsManager::sColorBufferFormat;
	for (std::size_t i = psoParams.mNumRenderTargets; i < rtCount; ++i) {
		psoParams.mRtFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoParams.mTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSOCreator::CreatePSO(psoParams, sPSO, sRootSign);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSign != nullptr);
}

void AmbientLightCmdListRecorder::Init(
	ID3D12Resource& baseColorMetalMaskBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	ID3D12Resource& ambientAccessibilityBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& ambientAccessibilityBufferRTCpuDesc) noexcept
{
	ASSERT(ValidateData() == false);

	mColorBufferCpuDesc = colorBufferCpuDesc;
	mAmbientAccessibilityBufferRTCpuDesc = ambientAccessibilityBufferRTCpuDesc;

	BuildBuffers(baseColorMetalMaskBuffer, ambientAccessibilityBuffer);

	ASSERT(ValidateData());
}

void AmbientLightCmdListRecorder::RecordAndPushCommandLists() noexcept {
	ASSERT(ValidateData());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSign != nullptr);

	static std::uint32_t currFrameIndex = 0U;

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[currFrameIndex] };
	ASSERT(cmdAlloc != nullptr);
	
	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, sPSO));

	mCmdList->RSSetViewports(1U, &SettingsManager::sScreenViewport);
	mCmdList->RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	mCmdList->OMSetRenderTargets(1U, &mColorBufferCpuDesc, false, nullptr);

	ID3D12DescriptorHeap* heaps[] = { &DescriptorManager::Get().GetCbvSrcUavDescriptorHeap() };
	mCmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	mCmdList->SetGraphicsRootSignature(sRootSign);
	
	// Set root parameters
	mCmdList->SetGraphicsRootDescriptorTable(0U, mBaseColor_MetalMaskGpuDesc);

	// Draw
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCmdList->DrawInstanced(6U, 1U, 0U, 0U);

	mCmdList->Close();

	mCmdListQueue.push(mCmdList);

	// Next frame
	currFrameIndex = (currFrameIndex + 1) % SettingsManager::sQueuedFrameCount;
}

bool AmbientLightCmdListRecorder::ValidateData() const noexcept {

	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAlloc[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		mCmdList != nullptr &&
		mColorBufferCpuDesc.ptr != 0UL &&
		mAmbientAccessibilityBufferRTCpuDesc.ptr != 0UL &&
		mBaseColor_MetalMaskGpuDesc.ptr != 0UL;

	return result;
}

void AmbientLightCmdListRecorder::BuildBuffers(
	ID3D12Resource& baseColorMetalMaskBuffer, 
	ID3D12Resource& ambientAccessibilityBuffer) noexcept {

	ASSERT(mBaseColor_MetalMaskGpuDesc.ptr == 0UL);
	
	// Create baseColor_metalMask buffer texture descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = baseColorMetalMaskBuffer.GetDesc().Format;
	srvDesc.Texture2D.MipLevels = baseColorMetalMaskBuffer.GetDesc().MipLevels;
	mBaseColor_MetalMaskGpuDesc = DescriptorManager::Get().CreateShaderResourceView(baseColorMetalMaskBuffer, srvDesc);

	// Create ambient accessibility buffer texture descriptor
	srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = ambientAccessibilityBuffer.GetDesc().Format;
	srvDesc.Texture2D.MipLevels = ambientAccessibilityBuffer.GetDesc().MipLevels;
	DescriptorManager::Get().CreateShaderResourceView(ambientAccessibilityBuffer, srvDesc);
}