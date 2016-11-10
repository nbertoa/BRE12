#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <tbb/concurrent_queue.h>

#include <DXUtils/D3DFactory.h>
#include <GlobalData/Settings.h>
#include <ResourceManager/BufferCreator.h>

struct FrameCBuffer;
class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command lists for deferred shading light pass.
// Steps:
// - Inherit from it and reimplement RecordAndPushCommandLists() method
// - Call RecordAndPushCommandLists() to create command lists to execute in the GPU
class LightingPassCmdListRecorder {
public:
	struct GeometryData {
		GeometryData() = default;

		BufferCreator::VertexBufferData mVertexBufferData;
		BufferCreator::IndexBufferData mIndexBufferData;
		std::vector<DirectX::XMFLOAT4X4> mWorldMatrices;
	};

	explicit LightingPassCmdListRecorder(ID3D12Device& device);
	virtual ~LightingPassCmdListRecorder() {}

	LightingPassCmdListRecorder(const LightingPassCmdListRecorder&) = delete;
	const LightingPassCmdListRecorder& operator=(const LightingPassCmdListRecorder&) = delete;
	LightingPassCmdListRecorder(LightingPassCmdListRecorder&&) = default;
	LightingPassCmdListRecorder& operator=(LightingPassCmdListRecorder&&) = default;

	// This method must be called before RecordAndPushCommandLists()
	virtual void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		const void* lights,
		const std::uint32_t numLights) noexcept = 0;

	// This method must be called before calling RecordAndPushCommandLists()
	void InitInternal(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		const D3D12_CPU_DESCRIPTOR_HANDLE colorBufferCpuDesc,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	// Record command lists and push them to the queue.
	virtual void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept = 0;

	// This method validates all data (nullptr's, etc)
	// When you inherit from this class, you should reimplement it to include
	// new members
	virtual bool ValidateData() const noexcept;

protected:
	ID3D12Device& mDevice;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrFrameIndex{ 0U };

	// Base command data. Once you inherits from this class, you should add
	// more class members that represent the extra information you need (like resources, for example)
	ID3D12DescriptorHeap* mCbvSrvUavDescHeap{ nullptr };

	// Where we push recorded command lists
	tbb::concurrent_queue<ID3D12CommandList*>* mCmdListQueue{ nullptr };

	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferCpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };

	std::uint32_t mNumLights{ 0U };

	UploadBuffer* mFrameCBuffer[Settings::sQueuedFrameCount]{ nullptr };
	UploadBuffer* mImmutableCBuffer{ nullptr };

	UploadBuffer* mLightsBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mLightsBufferGpuDescHandleBegin;
};