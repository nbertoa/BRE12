#include "RenderTask.h"

#include <PSOManager/PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager/RootSignatureManager.h>
#include <ShaderManager/ShaderManager.h>
#include <Utils/DebugUtils.h>

bool RenderTaskInitData::ValidateData() const {
	return 
		mRootSignName != nullptr && 
		!mMeshDataVec.empty() && 
		mPSOName != nullptr &&
		!mInputLayout.empty();
}

RenderTask::RenderTask(ID3D12Device* device)
	: mDevice(device)
{
	ASSERT(mDevice);
}

void RenderTask::Init(const RenderTaskInitData& initData) noexcept {
	ASSERT(initData.ValidateData());

	BuildRootSignature(initData.mRootSignName, initData.mRootSignDesc);
	BuildPSO(initData);
	BuildCommandObjects();
}

void RenderTask::BuildRootSignature(const char* rootSignName, const D3D12_ROOT_SIGNATURE_DESC& rootSignDesc) noexcept {
	ASSERT(mRootSign == nullptr);
	RootSignatureManager::gManager->CreateRootSignature(rootSignName, rootSignDesc, mRootSign);
}

void RenderTask::BuildPSO(const RenderTaskInitData& initData) noexcept {
	ASSERT(mRootSign != nullptr);
	ASSERT(mPSO == nullptr);
	ASSERT(initData.ValidateData());

	D3D12_SHADER_BYTECODE vertexShader{};
	if (initData.mVSFilename != nullptr) {
		ShaderManager::gManager->LoadShaderFile(initData.mVSFilename, vertexShader);
	}
	
	D3D12_SHADER_BYTECODE geomShader{};
	if (initData.mGSFilename != nullptr) {
		ShaderManager::gManager->LoadShaderFile(initData.mGSFilename, geomShader);
	}

	D3D12_SHADER_BYTECODE domainShader{};
	if (initData.mDSFilename != nullptr) {
		ShaderManager::gManager->LoadShaderFile(initData.mDSFilename, domainShader);
	}

	D3D12_SHADER_BYTECODE hullShader{};
	if (initData.mHSFilename != nullptr) {
		ShaderManager::gManager->LoadShaderFile(initData.mHSFilename, hullShader);
	}

	D3D12_SHADER_BYTECODE pixelShader{};
	if (initData.mPSFilename != nullptr) {
		ShaderManager::gManager->LoadShaderFile(initData.mPSFilename, pixelShader);
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.BlendState = initData.mBlendDesc;
	desc.DepthStencilState = initData.mDepthStencilDesc;
	desc.DS = domainShader;
	desc.DSVFormat = initData.mDepthStencilFormat;
	desc.GS = geomShader;
	desc.HS = hullShader;
	desc.InputLayout = { initData.mInputLayout.data(), (std::uint32_t)initData.mInputLayout.size() };
	desc.NumRenderTargets = initData.mNumRenderTargets;
	desc.PrimitiveTopologyType = mTopology;
	desc.pRootSignature = mRootSign;
	desc.PS = pixelShader;
	desc.RasterizerState = D3DFactory::DefaultRasterizerDesc();
	memcpy(desc.RTVFormats, initData.mRTVFormats, sizeof(initData.mRTVFormats));
	desc.SampleDesc = initData.mSampleDesc;
	desc.SampleMask = initData.mSampleMask;
	desc.VS = vertexShader;

	PSOManager::gManager->CreateGraphicsPSO(initData.mPSOName, desc, mPSO);
}

void RenderTask::BuildVertexAndIndexBuffers(
	GeometryData& geomData,
	const void* vertsData,
	const std::uint32_t numVerts,
	const std::size_t vertexSize,
	const void* indexData,
	const std::uint32_t numIndices) noexcept {

	ASSERT(geomData.mIndexBufferName != nullptr);
	ASSERT(geomData.mIndexBuffer == nullptr);
	ASSERT(geomData.mVertexBufferName != nullptr);
	ASSERT(geomData.mVertexBuffer == nullptr);
	ASSERT(vertsData != nullptr);
	ASSERT(numVerts > 0U);
	ASSERT(vertexSize > 0UL);
	ASSERT(indexData != nullptr);
	ASSERT(numIndices > 0U);
	ASSERT(mCmdList.Get() != nullptr);
	ASSERT(mCmdListAllocator.Get() != nullptr);

	std::uint32_t byteSize{ numVerts * (std::uint32_t)vertexSize };
	ResourceManager::gManager->CreateDefaultBuffer(geomData.mVertexBufferName, *mCmdList.Get(), vertsData, byteSize, geomData.mVertexBuffer, geomData.mUploadVertexBuffer);
	geomData.mVertexBufferView.BufferLocation = geomData.mVertexBuffer->GetGPUVirtualAddress();
	geomData.mVertexBufferView.SizeInBytes = byteSize;
	geomData.mVertexBufferView.StrideInBytes = (std::uint32_t)vertexSize;

	geomData.mIndexCount = numIndices;
	byteSize = numIndices * sizeof(std::uint32_t);
	ResourceManager::gManager->CreateDefaultBuffer(geomData.mIndexBufferName, *mCmdList.Get(), indexData, byteSize, geomData.mIndexBuffer, geomData.mUploadIndexBuffer);
	geomData.mIndexBufferView.BufferLocation = geomData.mIndexBuffer->GetGPUVirtualAddress();
	geomData.mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	geomData.mIndexBufferView.SizeInBytes = byteSize;
}

void RenderTask::BuildCommandObjects() noexcept {
	ASSERT(mDevice != nullptr);
	ASSERT(mCmdList.Get() == nullptr);
	ASSERT(mCmdListAllocator.Get() == nullptr);

	CHECK_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCmdListAllocator.GetAddressOf())));
	CHECK_HR(mDevice->CreateCommandList(0U, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdListAllocator.Get(), nullptr, IID_PPV_ARGS(mCmdList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCmdList->Close();
}