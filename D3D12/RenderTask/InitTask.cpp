#include "InitTask.h"

#include <PSOManager/PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager/RootSignatureManager.h>
#include <ShaderManager/ShaderManager.h>
#include <Utils/DebugUtils.h>

bool InitTaskInput::ValidateData() const {
	return !mMeshInfoVec.empty() && !mInputLayout.empty();
}

InitTask::InitTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect)
	: mDevice(device)
	, mViewport(screenViewport)
	, mScissorRect(scissorRect)
{
	ASSERT(device != nullptr);
}

void InitTask::Init(const InitTaskInput& input, tbb::concurrent_vector<ID3D12CommandList*>& /*cmdLists*/, RenderTaskInput& output) noexcept {
	ASSERT(input.ValidateData());
	
	output.mTopology = input.mTopology;

	BuildRootSignature(input.mRootSignDesc, output.mRootSign);
	BuildPSO(input, *output.mRootSign, output.mPSO);
	BuildCommandObjects(output.mCmdList, output.mCmdListAllocator);
}

void InitTask::BuildRootSignature(const D3D12_ROOT_SIGNATURE_DESC& rootSignDesc, ID3D12RootSignature* &rootSign) noexcept {
	ASSERT(rootSign == nullptr);
	RootSignatureManager::gManager->CreateRootSignature(rootSignDesc, rootSign);
}

void InitTask::BuildPSO(const InitTaskInput& input, ID3D12RootSignature& rootSign, ID3D12PipelineState* &pso) noexcept {
	ASSERT(pso == nullptr);
	ASSERT(input.ValidateData());

	D3D12_SHADER_BYTECODE vertexShader{};
	if (input.mVSFilename != nullptr) {
		ShaderManager::gManager->LoadShaderFile(input.mVSFilename, vertexShader);
	}

	D3D12_SHADER_BYTECODE geomShader{};
	if (input.mGSFilename != nullptr) {
		ShaderManager::gManager->LoadShaderFile(input.mGSFilename, geomShader);
	}

	D3D12_SHADER_BYTECODE domainShader{};
	if (input.mDSFilename != nullptr) {
		ShaderManager::gManager->LoadShaderFile(input.mDSFilename, domainShader);
	}

	D3D12_SHADER_BYTECODE hullShader{};
	if (input.mHSFilename != nullptr) {
		ShaderManager::gManager->LoadShaderFile(input.mHSFilename, hullShader);
	}

	D3D12_SHADER_BYTECODE pixelShader{};
	if (input.mPSFilename != nullptr) {
		ShaderManager::gManager->LoadShaderFile(input.mPSFilename, pixelShader);
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.BlendState = input.mBlendDesc;
	desc.DepthStencilState = input.mDepthStencilDesc;
	desc.DS = domainShader;
	desc.DSVFormat = input.mDepthStencilFormat;
	desc.GS = geomShader;
	desc.HS = hullShader;
	desc.InputLayout = { input.mInputLayout.data(), (std::uint32_t)input.mInputLayout.size() };
	desc.NumRenderTargets = input.mNumRenderTargets;
	desc.PrimitiveTopologyType = input.mTopology;
	desc.pRootSignature = &rootSign;
	desc.PS = pixelShader;
	desc.RasterizerState = D3DFactory::DefaultRasterizerDesc();
	memcpy(desc.RTVFormats, input.mRTVFormats, sizeof(input.mRTVFormats));
	desc.SampleDesc = input.mSampleDesc;
	desc.SampleMask = input.mSampleMask;
	desc.VS = vertexShader;

	PSOManager::gManager->CreateGraphicsPSO(desc, pso);
}

void InitTask::BuildVertexAndIndexBuffers(
	GeometryData& geomData,
	const void* vertsData,
	const std::uint32_t numVerts,
	const std::size_t vertexSize,
	const void* indexData,
	const std::uint32_t numIndices,
	ID3D12GraphicsCommandList& cmdList) noexcept {

	ASSERT(geomData.mIndexBuffer == nullptr);
	ASSERT(geomData.mVertexBuffer == nullptr);
	ASSERT(vertsData != nullptr);
	ASSERT(numVerts > 0U);
	ASSERT(vertexSize > 0UL);
	ASSERT(indexData != nullptr);
	ASSERT(numIndices > 0U);

	std::uint32_t byteSize{ numVerts * (std::uint32_t)vertexSize };

	ResourceManager::gManager->CreateDefaultBuffer(cmdList, vertsData, byteSize, geomData.mVertexBuffer, geomData.mUploadVertexBuffer);
	geomData.mVertexBufferView.BufferLocation = geomData.mVertexBuffer->GetGPUVirtualAddress();
	geomData.mVertexBufferView.SizeInBytes = byteSize;
	geomData.mVertexBufferView.StrideInBytes = (std::uint32_t)vertexSize;

	geomData.mIndexCount = numIndices;
	byteSize = numIndices * sizeof(std::uint32_t);
	ResourceManager::gManager->CreateDefaultBuffer(cmdList, indexData, byteSize, geomData.mIndexBuffer, geomData.mUploadIndexBuffer);
	geomData.mIndexBufferView.BufferLocation = geomData.mIndexBuffer->GetGPUVirtualAddress();
	geomData.mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	geomData.mIndexBufferView.SizeInBytes = byteSize;
}

void InitTask::BuildCommandObjects(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& cmdList, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& cmdListAlloc) noexcept {
	ASSERT(mDevice != nullptr);
	ASSERT(cmdList == nullptr);
	ASSERT(cmdListAlloc == nullptr);

	CHECK_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdListAlloc.GetAddressOf())));
	CHECK_HR(mDevice->CreateCommandList(0U, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdListAlloc.Get(), nullptr, IID_PPV_ARGS(cmdList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	cmdList->Close();
}