#pragma once

#include <memory>
#include <vector>

#include <GlobalData\Settings.h>
#include <GeometryPass\GeometryPassCmdListRecorder.h>

class CommandListExecutor;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12DescriptorHeap;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Pass responsible to execute recorders related with deferred shading geometry pass
class GeometryPass {
public:
	// Geometry buffers
	enum Buffers {
		NORMAL_SMOOTHNESS = 0U, // 2 encoded normals based on octahedron encoding + 1 smoothness
		BASECOLOR_METALMASK, // 3 base color + 1 metal mask
		BUFFERS_COUNT
	};

	using Recorders = std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>;

	GeometryPass() = default;
	~GeometryPass() = default;
	GeometryPass(const GeometryPass&) = delete;
	const GeometryPass& operator=(const GeometryPass&) = delete;
	GeometryPass(GeometryPass&&) = delete;
	GeometryPass& operator=(GeometryPass&&) = delete;

	// You should get recorders and fill them, before calling Init()
	__forceinline Recorders& GetRecorders() noexcept { return mRecorders; }

	// You should call this method after filling recorders and before Execute()
	void Init(
		ID3D12Device& device,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc,
		CommandListExecutor& cmdListProcessor,
		ID3D12CommandQueue& cmdQueue) noexcept;
	
	// Get geometry buffers
	__forceinline Microsoft::WRL::ComPtr<ID3D12Resource>* GetBuffers() noexcept { return mBuffers; }
	
	void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	void ExecuteBeginTask() noexcept;

	CommandListExecutor* mCmdListProcessor{ nullptr };
	ID3D12CommandQueue* mCmdQueue{ nullptr };

	// 1 command allocater per queued frame.	
	ID3D12CommandAllocator* mCmdAllocs[Settings::sQueuedFrameCount]{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	// Geometry buffers data
	Microsoft::WRL::ComPtr<ID3D12Resource> mBuffers[BUFFERS_COUNT];
	D3D12_CPU_DESCRIPTOR_HANDLE mRtvCpuDescs[BUFFERS_COUNT];
	ID3D12DescriptorHeap* mDescHeap{ nullptr };

	// Depth buffer cpu descriptor
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };

	// Geometry buffers cpu descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE mGeometryBuffersCpuDescs[BUFFERS_COUNT]{ 0UL };
	
	Recorders mRecorders;
};
