#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <tbb/concurrent_queue.h>

#include <GlobalData/Settings.h>
#include <MathUtils\MathUtils.h>
#include <ResourceManager/BufferCreator.h>

struct FrameCBuffer;
class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for sky box pass.
class SkyBoxCmdListRecorder {
public:
	explicit SkyBoxCmdListRecorder(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);
	~SkyBoxCmdListRecorder() = default;
	SkyBoxCmdListRecorder(const SkyBoxCmdListRecorder&) = delete;
	const SkyBoxCmdListRecorder& operator=(const SkyBoxCmdListRecorder&) = delete;
	SkyBoxCmdListRecorder(SkyBoxCmdListRecorder&&) = delete;
	SkyBoxCmdListRecorder& operator=(SkyBoxCmdListRecorder&&) = delete;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	// This method must be called before RecordAndPushCommandLists()
	void Init(
		const BufferCreator::VertexBufferData& vertexBufferData, 
		const BufferCreator::IndexBufferData indexBufferData,
		const DirectX::XMFLOAT4X4& worldMatrix,
		ID3D12Resource& cubeMap,
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(ID3D12Resource& cubeMap) noexcept;

	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrFrameIndex{ 0U };
	
	BufferCreator::VertexBufferData mVertexBufferData;
	BufferCreator::IndexBufferData mIndexBufferData;
	DirectX::XMFLOAT4X4 mWorldMatrix{ MathUtils::Identity4x4() };

	UploadBuffer* mFrameCBuffer[Settings::sQueuedFrameCount]{ nullptr };

	UploadBuffer* mObjectCBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mObjectCBufferGpuDescHandleBegin;

	D3D12_GPU_DESCRIPTOR_HANDLE mCubeMapBufferGpuDescHandleBegin;

	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferCpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };
};