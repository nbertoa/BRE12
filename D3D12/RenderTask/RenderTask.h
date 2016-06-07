#pragma once

#include <cstdint>
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <tbb/concurrent_vector.h>
#include <wrl.h>

#include <DXUtils/D3DFactory.h>
#include <GeometryGenerator/GeometryGenerator.h>
#include <MathUtils/MathHelper.h>
#include <Utils/DebugUtils.h>

class UploadBuffer;

struct GeometryData {
	GeometryData() = default;

	DirectX::XMFLOAT4X4 mWorld{ MathHelper::Identity4x4() };

	ID3D12Resource* mVertexBuffer{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};

	ID3D12Resource* mIndexBuffer{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};

	std::uint32_t mIndexCount{ 0U };
	std::uint32_t mStartIndexLoc{ 0U };
	std::uint32_t mBaseVertexLoc{ 0U };
};


struct RenderTaskInput {
	ID3D12Device* mDevice{ nullptr };
	D3D12_VIEWPORT mViewport{};
	D3D12_RECT mScissorRect{};

	ID3D12DescriptorHeap* mCBVHeap{ nullptr };
	ID3D12RootSignature* mRootSign{ nullptr };
	ID3D12PipelineState* mPSO{ nullptr };

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCmdList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCmdListAllocator;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };

	using GeometryDataVec = std::vector<GeometryData>;
	GeometryDataVec mGeomDataVec{};

	UploadBuffer* mObjectConstants{ nullptr };
};

struct MeshInfo {
	explicit MeshInfo() = default;
	explicit MeshInfo(const GeometryGenerator::MeshData* meshData, const DirectX::XMFLOAT4X4& world)
		: mData(meshData)
		, mWorld(world)
	{
		ASSERT(meshData != nullptr);
	}

	const GeometryGenerator::MeshData* mData;
	DirectX::XMFLOAT4X4 mWorld{ MathHelper::Identity4x4() };
};

// Fill this data and pass to RenderTask::Init()
struct RenderTaskInitData {
	RenderTaskInitData() = default;
	RenderTaskInitData(const RenderTaskInitData&) = delete;
	const RenderTaskInitData& operator=(const RenderTaskInitData&) = delete;
	
	bool ValidateData() const;

	D3D12_ROOT_SIGNATURE_DESC mRootSignDesc{};
	
	using MeshInfoVec = std::vector<MeshInfo>;
	MeshInfoVec mMeshInfoVec{};

	// If a shader filename is nullptr, then we do not load it.
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

// - Inherit from this class and reimplement virtual methods.
class RenderTask {
public:
	explicit RenderTask(const char* taskName, ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect);
	RenderTask(const RenderTask&) = delete;
	const RenderTask& operator=(const RenderTask&) = delete;

	// Store new initialization commands in cmdLists
	virtual void Init(const RenderTaskInitData& initData, tbb::concurrent_vector<ID3D12CommandList*>& cmdLists) noexcept;
	virtual void Update() noexcept {};
	
	// Store new command lists in cmdLists  
	virtual void BuildCmdLists(
		tbb::concurrent_vector<ID3D12CommandList*>& cmdLists, 
		const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept = 0;

protected:
	void BuildRootSignature(const D3D12_ROOT_SIGNATURE_DESC& rootSignDesc) noexcept;
	void BuildPSO(const RenderTaskInitData& initData) noexcept;
	void BuildVertexAndIndexBuffers(
		GeometryData& geomData, 
		const void* vertsData, 
		const std::uint32_t numVerts, 
		const std::size_t vertexSize, 
		const void* indexData,
		const std::uint32_t numIndices) noexcept;
	void BuildCommandObjects() noexcept;
	
	std::string mTaskName;

	ID3D12Device* mDevice{ nullptr };

	D3D12_VIEWPORT mViewport{};
	D3D12_RECT mScissorRect{};

	ID3D12DescriptorHeap* mCBVHeap{ nullptr };
	ID3D12RootSignature* mRootSign{ nullptr };
	ID3D12PipelineState* mPSO{ nullptr };
	
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCmdList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCmdListAllocator;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
	
	using GeometryDataVec = std::vector<GeometryData>;
	GeometryDataVec mGeomDataVec{};

	UploadBuffer* mObjectConstants{ nullptr };
};