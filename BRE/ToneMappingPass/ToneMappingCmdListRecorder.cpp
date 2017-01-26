#include "ToneMappingCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandManager/CommandManager.h>
#include <DescriptorManager\DescriptorManager.h>
#include <PSOCreator/PSOCreator.h>
#include <Utils/DebugUtils.h>

// Root Signature:
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL)" 0 -> Color Buffer Texture

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

ToneMappingCmdListRecorder::ToneMappingCmdListRecorder(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: mCmdListQueue(cmdListQueue)
{
	BuildCommandObjects(mCmdList, mCmdAlloc, _countof(mCmdAlloc));
}

void ToneMappingCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSign == nullptr);

	// Build pso and root signature
	PSOCreator::PSOParams psoParams{};
	const std::size_t rtCount{ _countof(psoParams.mRtFormats) };
	psoParams.mDepthStencilDesc = D3DFactory::GetDisabledDepthStencilDesc();
	psoParams.mPSFilename = "ToneMappingPass/Shaders/PS.cso";
	psoParams.mRootSignFilename = "ToneMappingPass/Shaders/RS.cso";
	psoParams.mVSFilename = "ToneMappingPass/Shaders/VS.cso";
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

void ToneMappingCmdListRecorder::Init(
	ID3D12Resource& inputColorBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputBufferCpuDesc) noexcept
{
	ASSERT(IsDataValid() == false);

	mOutputColorBufferCpuDesc = outputBufferCpuDesc;

	BuildBuffers(inputColorBuffer);

	ASSERT(IsDataValid());
}

void ToneMappingCmdListRecorder::RecordAndPushCommandLists() noexcept {
	ASSERT(IsDataValid());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSign != nullptr);

	static std::uint32_t currFrameIndex = 0U;

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[currFrameIndex] };
	ASSERT(cmdAlloc != nullptr);
	
	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, sPSO));

	mCmdList->RSSetViewports(1U, &SettingsManager::sScreenViewport);
	mCmdList->RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	mCmdList->OMSetRenderTargets(1U, &mOutputColorBufferCpuDesc, false, nullptr);

	ID3D12DescriptorHeap* heaps[] = { &DescriptorManager::Get().GetCbvSrcUavDescriptorHeap() };
	mCmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	mCmdList->SetGraphicsRootSignature(sRootSign);
	
	// Set root parameters
	mCmdList->SetGraphicsRootDescriptorTable(0U, mInputColorBufferGpuDesc);

	// Draw object	
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCmdList->DrawInstanced(6U, 1U, 0U, 0U);

	mCmdList->Close();

	mCmdListQueue.push(mCmdList);

	// Next frame
	currFrameIndex = (currFrameIndex + 1) % SettingsManager::sQueuedFrameCount;
}

bool ToneMappingCmdListRecorder::IsDataValid() const noexcept {

	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAlloc[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		mCmdList != nullptr &&
		mInputColorBufferGpuDesc.ptr != 0UL &&
		mOutputColorBufferCpuDesc.ptr != 0UL;

	return result;
}

void ToneMappingCmdListRecorder::BuildBuffers(ID3D12Resource& colorBuffer) noexcept {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc[1U]{};
	ID3D12Resource* res[1] = {
		&colorBuffer,
	};

	// Create color buffer texture descriptor
	srvDesc[0].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc[0].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc[0].Texture2D.MostDetailedMip = 0;
	srvDesc[0].Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc[0].Format = colorBuffer.GetDesc().Format;
	srvDesc[0].Texture2D.MipLevels = colorBuffer.GetDesc().MipLevels;

	mInputColorBufferGpuDesc = DescriptorManager::Get().CreateShaderResourceViews(res, srvDesc, _countof(srvDesc));
}