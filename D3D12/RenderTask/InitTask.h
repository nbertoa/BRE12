#pragma once

#include <cstdint>
#include <d3d12.h>
#include <DirectXMath.h>
#include <tbb/concurrent_queue.h>
#include <wrl.h>

#include <DXUtils/D3DFactory.h>
#include <GeometryGenerator/GeometryGenerator.h>
#include <MathUtils/MathHelper.h>
#include <RenderTask/CmdBuilderTask.h>
#include <Utils/DebugUtils.h>

class UploadBuffer;

// Geometry information used in InitTaskInput to build vertex/index buffers
// Take into account that internal data should be valid at least until InitTaskInput's InitTask
// is executed.
struct GeometryInfo {
	GeometryInfo() = default;
	explicit GeometryInfo(const void* verts, const std::uint32_t numVerts, const void* indices, const std::uint32_t numIndices, DirectX::XMFLOAT4X4& world)
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

struct InitTaskInput {
	InitTaskInput() = default;

	bool ValidateData() const;

	using MeshInfoVec = std::vector<GeometryInfo>;
	MeshInfoVec mMeshInfoVec{};

	// If a shader filename is nullptr, then we do not load it.
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout{};
	const char* mRootSignFilename{ nullptr };
	const char* mVSFilename{ nullptr };
	const char* mGSFilename{ nullptr };
	const char* mDSFilename{ nullptr };
	const char* mHSFilename{ nullptr };
	const char* mPSFilename{ nullptr };

	D3D12_BLEND_DESC mBlendDesc = D3DFactory::DefaultBlendDesc();
	D3D12_RASTERIZER_DESC mRasterizerDesc = D3DFactory::DefaultRasterizerDesc();
	D3D12_DEPTH_STENCIL_DESC mDepthStencilDesc = D3DFactory::DefaultDepthStencilDesc();
	std::uint32_t mNumRenderTargets{ 1U };
	DXGI_SAMPLE_DESC mSampleDesc{ 1U, 0U };
	std::uint32_t mSampleMask{ UINT_MAX };
	D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
};

// Task that given its InitTaskInput, initializes a CmdBuilderTaskOutput (that will be used to build command lists)
// Steps:
// - Inherit from InitTask and reimplement InitCmdBuilders() method (you should call InitTask::InitCmdBuilders() method in reimplementation)
// - Fill InitTaskInput data (InitTask::TaskInput())
// - Call InitTask::InitCmdBuilders() to fill CmdBuilderTaskInput accordingly.
class InitTask {
public:
	InitTask() = default;

	__forceinline InitTaskInput& TaskInput() noexcept { return mInput; }

	// We require to pass the queue of command lists for initialization purposes (create resources, buffers, etc)
	virtual void InitCmdBuilders(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdLists, CmdBuilderTaskInput& output) noexcept;

protected:
	void BuildPSO(ID3D12RootSignature* &rootSign, ID3D12PipelineState* &pso) noexcept;
	void BuildVertexAndIndexBuffers(
		GeometryData& geomData,
		const void* vertsData,
		const std::uint32_t numVerts,
		const std::size_t vertexSize,
		const void* indexData,
		const std::uint32_t numIndices,
		ID3D12GraphicsCommandList& cmdList) noexcept;

	void BuildCommandObjects(CmdBuilderTaskInput& output) noexcept;

	InitTaskInput mInput{};
};