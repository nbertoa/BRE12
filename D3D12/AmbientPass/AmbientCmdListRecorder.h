#pragma once

#include <tbb/concurrent_queue.h>

#include <GlobalData/Settings.h>
#include <ResourceManager/BufferCreator.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12DescriptorHeap;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListProcessor.
// This class has common data and functionality to record command list for ambient light pass.
class AmbientCmdListRecorder {
public:
	explicit AmbientCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(
		const BufferCreator::VertexBufferData& vertexBufferData,
		const BufferCreator::IndexBufferData indexBufferData,
		ID3D12Resource& baseColorMetalMaskBuffer) noexcept;

	void RecordCommandLists(
		const D3D12_CPU_DESCRIPTOR_HANDLE& rtvCpuDescHandle,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(ID3D12Resource& colorBuffer) noexcept;

	ID3D12Device& mDevice;
	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrFrameIndex{ 0U };

	ID3D12DescriptorHeap* mCbvSrvUavDescHeap{ nullptr };

	BufferCreator::VertexBufferData mVertexBufferData;
	BufferCreator::IndexBufferData mIndexBufferData;
};