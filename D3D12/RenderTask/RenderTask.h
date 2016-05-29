#pragma once

#include <cstdint>
#include <d3d12.h>
#include <DirectXMath.h>
#include <tbb/concurrent_vector.h>

#include <DXUtils/D3DFactory.h>
#include <GeometryGenerator/GeometryGenerator.h>
#include <MathUtils/MathHelper.h>

// Fill this data and pass to RenderTask instance
struct RenderTaskInitData {
	RenderTaskInitData() = default;

	const char* mRootSignName{ nullptr };
	D3D12_ROOT_SIGNATURE_DESC mRootSignDesc{};
	
	using MeshDataVec = std::vector<GeometryGenerator::MeshData>;
	MeshDataVec mMeshDataVec{};

	// If a vertex shader filename is nullptr, then we do not load it.
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout{};
	const char* mVSFilename{ nullptr };
	const char* mGSFilename{ nullptr };
	const char* mDSFilename{ nullptr };
	const char* mHSFilename{ nullptr };
	const char* mPSFilename{ nullptr };

	D3D12_BLEND_DESC mBlendDesc = D3DFactory::DefaultBlendDesc();
	D3D12_RASTERIZER_DESC mRasterizerDesc = D3DFactory::DefaultRasterizerDesc();
	D3D12_DEPTH_STENCIL_DESC mDepthStencilDesc = D3DFactory::DefaultDepthStencilDesc();
	DXGI_FORMAT mDepthStencilFormat{ DXGI_FORMAT_D24_UNORM_S8_UINT };
	std::uint32_t mNumRenderTargets{ 1U };
	DXGI_FORMAT mRTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{ DXGI_FORMAT_R8G8B8A8_UNORM };
	DXGI_SAMPLE_DESC mSampleDesc{1U, 0U};
	std::uint32_t mSampleMask{ UINT_MAX };
};

// - Inherit from this class and implement virtual methods.
class RenderTask {
public:
	explicit RenderTask() {};

	virtual void Init(const RenderTaskInitData& /*initData*/) noexcept {};
	virtual void Update() noexcept {};
	
	// Store new command lists in cmdLists  
	virtual void BuildCmdLists(tbb::concurrent_vector<ID3D12CommandList*>& /*cmdLists*/) noexcept {};
	virtual void Destroy() noexcept {};	

private:
	struct GeometryData {
		GeometryData() = default;

		DirectX::XMFLOAT4X4 mWorld{ MathHelper::Identity4x4() };

		ID3D12Resource* mVertexBuffer{ nullptr };
		ID3D12Resource* mIndexBuffer{ nullptr };

		std::uint32_t mIndexCount{ 0U };
		std::uint32_t mStartIndexLoc{ 0U };
		std::uint32_t mBaseVertexLoc{ 0U };
	};

	ID3D12RootSignature* mRootSign{ nullptr };
	ID3D12PipelineState* mPSO{ nullptr };
	D3D12_PRIMITIVE_TOPOLOGY mTopology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	
	using GeometryDataVec = std::vector<GeometryData>;
	GeometryDataVec mGeomDataVec{};
};