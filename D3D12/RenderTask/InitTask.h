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
#include <RenderTask/RenderTask.h>
#include <Utils/DebugUtils.h>

class UploadBuffer;

struct InitTaskInput {
	struct MeshInfo {
		explicit MeshInfo() = default;
		explicit MeshInfo(const void* verts, const std::uint32_t numVerts, const void* indices, const std::uint32_t numIndices, DirectX::XMFLOAT4X4& world)
			: mVerts(verts)
			, mNumVerts(numVerts)
			, mIndices(indices)
			, mNumIndices(numIndices)
			, mWorld(world)
		{
			ASSERT(mVerts != nullptr);
			ASSERT(mNumVerts > 0U);
			ASSERT(mIndices != nullptr);
			ASSERT(mNumIndices > 0U);
		}

		bool ValidateData() const {
			return mVerts != nullptr && mNumVerts > 0U && mIndices != nullptr && mNumIndices > 0U;
		}

		const void* mVerts{ nullptr };
		std::uint32_t mNumVerts{ 0U };
		const void* mIndices{ nullptr };
		std::uint32_t mNumIndices{ 0U };
		DirectX::XMFLOAT4X4 mWorld{ MathHelper::Identity4x4() };
	};

	InitTaskInput() = default;
	InitTaskInput(const InitTaskInput&) = delete;
	const InitTaskInput& operator=(const InitTaskInput&) = delete;

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
	DXGI_SAMPLE_DESC mSampleDesc{ 1U, 0U };
	std::uint32_t mSampleMask{ UINT_MAX };
	D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
};

// - Inherit from this class and reimplement virtual methods.
class InitTask {
public:
	explicit InitTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect);
	InitTask(const InitTask&) = delete;
	const InitTask& operator=(const InitTask&) = delete;

	// Store new initialization commands in cmdLists
	virtual void Init(const InitTaskInput& input, tbb::concurrent_vector<ID3D12CommandList*>& cmdLists, RenderTaskInput& output) noexcept;

protected:
	void BuildRootSignature(const D3D12_ROOT_SIGNATURE_DESC& rootSignDesc, ID3D12RootSignature* &rootSign) noexcept;
	void BuildPSO(const InitTaskInput& input, ID3D12RootSignature& signRoot, ID3D12PipelineState* &pso) noexcept;
	void BuildVertexAndIndexBuffers(
		GeometryData& geomData,
		const void* vertsData,
		const std::uint32_t numVerts,
		const std::size_t vertexSize,
		const void* indexData,
		const std::uint32_t numIndices,
		ID3D12GraphicsCommandList& cmdList) noexcept;
	void BuildCommandObjects(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& cmdList, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& cmdListAlloc) noexcept;
	
	ID3D12Device* mDevice{ nullptr };
	D3D12_VIEWPORT mViewport{};
	D3D12_RECT mScissorRect{};
};