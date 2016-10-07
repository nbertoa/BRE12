#pragma once

#include <memory>
#include <vector>

#include <GlobalData\Settings.h>
#include <GeometryPass\GeometryPassCmdListRecorder.h>

class CommandListProcessor;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12DescriptorHeap;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

class GeometryPass {
public:
	enum Buffers {
		NORMAL_SMOOTHNESS_DEPTH = 0U,
		BASECOLOR_METALMASK,
		SPECULARREFLECTION,
		BUFFERS_COUNT
	};

	using Recorders = std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>;

	GeometryPass() = default;
	GeometryPass(const GeometryPass&) = delete;
	const GeometryPass& operator=(const GeometryPass&) = delete;

	// You should get recorders and fill them, before calling Init()
	__forceinline Recorders& GetRecorders() noexcept { return mRecorders; }

	// You should call this method after filling recorders and before Execute()
	void Init(ID3D12Device& device) noexcept;

	// Geometry buffers formats. It has a size of D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT
	__forceinline static const DXGI_FORMAT* BufferFormats() noexcept { return sBufferFormats; }

	// Get geometry buffers
	__forceinline Microsoft::WRL::ComPtr<ID3D12Resource>* GetBuffers() noexcept { return mBuffers; }
	
	// This method expects geometry buffers in PRESENT state.
	// depthCpuDesc: Cpu descriptor handle of the depth stencil buffer
	void Execute(
		CommandListProcessor& cmdListProcessor, 
		ID3D12CommandQueue& cmdQueue, 
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthCpuDesc,
		const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Buffer formats
	static const DXGI_FORMAT sBufferFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];

	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	// 1 command allocater per queued frame.	
	ID3D12CommandAllocator* mCmdAllocs[Settings::sQueuedFrameCount]{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	// Geometry buffers data
	Microsoft::WRL::ComPtr<ID3D12Resource> mBuffers[BUFFERS_COUNT];
	D3D12_CPU_DESCRIPTOR_HANDLE mRtvCpuDescs[BUFFERS_COUNT];
	ID3D12DescriptorHeap* mDescHeap{ nullptr };
	
	Recorders mRecorders;
};
