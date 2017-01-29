#include "ToneMappingCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <CommandManager/CommandAllocatorManager.h>
#include <CommandManager/CommandListManager.h>
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

	void BuildCommandObjects(
		ID3D12GraphicsCommandList* &commandList, 
		ID3D12CommandAllocator* commandAllocators[], 
		const std::size_t commandAllocatorCount) noexcept 
	{
		ASSERT(commandList == nullptr);

#ifdef _DEBUG
		for (std::uint32_t i = 0U; i < commandAllocatorCount; ++i) {
			ASSERT(commandAllocators[i] == nullptr);
		}
#endif

		for (std::uint32_t i = 0U; i < commandAllocatorCount; ++i) {
			CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[i]);
		}

		CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators[0], commandList);

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		commandList->Close();
	}
}

ToneMappingCmdListRecorder::ToneMappingCmdListRecorder() {
	BuildCommandObjects(mCommandList, mCommandAllocators, _countof(mCommandAllocators));
}

void ToneMappingCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSignature == nullptr);

	// Build pso and root signature
	PSOManager::PSOCreationData psoData{};
	const std::size_t rtCount{ _countof(psoData.mRenderTargetFormats) };
	psoData.mDepthStencilDescriptor = D3DFactory::GetDisabledDepthStencilDesc();

	ShaderManager::LoadShaderFile("ToneMappingPass/Shaders/PS.cso", psoData.mPixelShaderBytecode);
	ShaderManager::LoadShaderFile("ToneMappingPass/Shaders/VS.cso", psoData.mVertexShaderBytecode);

	ID3DBlob* rootSignatureBlob;
	ShaderManager::LoadShaderFile("ToneMappingPass/Shaders/RS.cso", rootSignatureBlob);
	RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob, psoData.mRootSignature);
	sRootSignature = psoData.mRootSignature;

	psoData.mNumRenderTargets = 1U;
	psoData.mRenderTargetFormats[0U] = SettingsManager::sColorBufferFormat;
	for (std::size_t i = psoData.mNumRenderTargets; i < rtCount; ++i) {
		psoData.mRenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSOManager::CreateGraphicsPSO(psoData, sPSO);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
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
	ASSERT(sRootSignature != nullptr);

	static std::uint32_t currentFrameIndex = 0U;

	ID3D12CommandAllocator* commandAllocator{ mCommandAllocators[currentFrameIndex] };
	ASSERT(commandAllocator != nullptr);
	
	CHECK_HR(commandAllocator->Reset());
	CHECK_HR(mCommandList->Reset(commandAllocator, sPSO));

	mCommandList->RSSetViewports(1U, &SettingsManager::sScreenViewport);
	mCommandList->RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	mCommandList->OMSetRenderTargets(1U, &mOutputColorBufferCpuDesc, false, nullptr);

	ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
	mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);
	mCommandList->SetGraphicsRootSignature(sRootSignature);
	
	// Set root parameters
	mCommandList->SetGraphicsRootDescriptorTable(0U, mInputColorBufferGpuDesc);

	// Draw object	
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->DrawInstanced(6U, 1U, 0U, 0U);

	mCommandList->Close();

	CommandListExecutor::Get().AddCommandList(*mCommandList);

	// Next frame
	currentFrameIndex = (currentFrameIndex + 1) % SettingsManager::sQueuedFrameCount;
}

bool ToneMappingCmdListRecorder::IsDataValid() const noexcept {
	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocators[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		mCommandList != nullptr &&
		mInputColorBufferGpuDesc.ptr != 0UL &&
		mOutputColorBufferCpuDesc.ptr != 0UL;

	return result;
}

void ToneMappingCmdListRecorder::BuildBuffers(ID3D12Resource& inputColorBuffer) noexcept {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor[1U]{};
	ID3D12Resource* resources[1] = {
		&inputColorBuffer,
	};

	// Create color buffer texture descriptor
	srvDescriptor[0].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescriptor[0].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDescriptor[0].Texture2D.MostDetailedMip = 0;
	srvDescriptor[0].Texture2D.ResourceMinLODClamp = 0.0f;
	srvDescriptor[0].Format = inputColorBuffer.GetDesc().Format;
	srvDescriptor[0].Texture2D.MipLevels = inputColorBuffer.GetDesc().MipLevels;

	mInputColorBufferGpuDesc = 
		CbvSrvUavDescriptorManager::CreateShaderResourceViews(
			resources, 
			srvDescriptor, 
			_countof(srvDescriptor));
}