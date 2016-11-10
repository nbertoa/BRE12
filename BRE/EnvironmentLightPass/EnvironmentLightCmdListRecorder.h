#pragma once

#include <tbb/concurrent_queue.h>

#include <GlobalData/Settings.h>
#include <ResourceManager/BufferCreator.h>

class UploadBuffer;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12DescriptorHeap;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for environment light pass.
class EnvironmentLightCmdListRecorder {
public:
	explicit EnvironmentLightCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	~EnvironmentLightCmdListRecorder() = default;
	EnvironmentLightCmdListRecorder(const EnvironmentLightCmdListRecorder&) = delete;
	const EnvironmentLightCmdListRecorder& operator=(const EnvironmentLightCmdListRecorder&) = delete;
	EnvironmentLightCmdListRecorder(EnvironmentLightCmdListRecorder&&) = default;
	EnvironmentLightCmdListRecorder& operator=(EnvironmentLightCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(
		const BufferCreator::VertexBufferData& vertexBufferData,
		const BufferCreator::IndexBufferData indexBufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers, 
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

	ID3D12Device& mDevice;
	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrFrameIndex{ 0U };

	ID3D12DescriptorHeap* mCbvSrvUavDescHeap{ nullptr };
	UploadBuffer* mImmutableCBuffer{ nullptr };
	UploadBuffer* mFrameCBuffer[Settings::sQueuedFrameCount]{ nullptr };

	BufferCreator::VertexBufferData mVertexBufferData;
	BufferCreator::IndexBufferData mIndexBufferData;

	D3D12_GPU_DESCRIPTOR_HANDLE mCubeMapsBufferGpuDescHandleBegin;

	// Color & Depth buffers cpu descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferCpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };
};