#include "PostProcessCmdListRecorder.h"

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
			commandAllocators[i] = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
		}

		commandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators[0]);

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		commandList->Close();
	}
}

PostProcessCmdListRecorder::PostProcessCmdListRecorder() {
	BuildCommandObjects(mCommandList, mCommandAllocators, _countof(mCommandAllocators));
}

void PostProcessCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSignature == nullptr);

	// Build pso and root signature
	PSOManager::PSOCreationData psoData{};
	const std::size_t rtCount{ _countof(psoData.mRenderTargetFormats) };
	psoData.mDepthStencilDescriptor = D3DFactory::GetDisabledDepthStencilDesc();

	psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("PostProcessPass/Shaders/PS.cso");
	psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("PostProcessPass/Shaders/VS.cso");

	ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("PostProcessPass/Shaders/RS.cso");
	psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
	sRootSignature = psoData.mRootSignature;

	psoData.mNumRenderTargets = 1U;
	psoData.mRenderTargetFormats[0U] = SettingsManager::sFrameBufferRTFormat;
	for (std::size_t i = psoData.mNumRenderTargets; i < rtCount; ++i) {
		psoData.mRenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	sPSO = &PSOManager::CreateGraphicsPSO(psoData);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
}

void PostProcessCmdListRecorder::Init(ID3D12Resource& inputColorBuffer) noexcept  {
	ASSERT(IsDataValid() == false);
	
	BuildBuffers(inputColorBuffer);

	ASSERT(IsDataValid());
}

void PostProcessCmdListRecorder::RecordAndPushCommandLists(const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept {
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
	mCommandList->OMSetRenderTargets(1U, &frameBufferCpuDesc, false, nullptr);

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

bool PostProcessCmdListRecorder::IsDataValid() const noexcept {
	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocators[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		mCommandList != nullptr &&
		mInputColorBufferGpuDesc.ptr != 0UL;

	return result;
}

void PostProcessCmdListRecorder::BuildBuffers(ID3D12Resource& inputColorBuffer) noexcept {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc[1U]{};
	ID3D12Resource* resources[1] = {
		&inputColorBuffer,
	};

	// Create color buffer texture descriptor
	srvDesc[0].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc[0].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc[0].Texture2D.MostDetailedMip = 0;
	srvDesc[0].Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc[0].Format = inputColorBuffer.GetDesc().Format;
	srvDesc[0].Texture2D.MipLevels = inputColorBuffer.GetDesc().MipLevels;

	mInputColorBufferGpuDesc = CbvSrvUavDescriptorManager::CreateShaderResourceViews(resources, srvDesc, _countof(srvDesc));
}