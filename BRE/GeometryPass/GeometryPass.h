#pragma once

#include <memory>
#include <vector>

#include <GeometryPass\GeometryPassCmdListRecorder.h>
#include <SettingsManager\SettingsManager.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
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

	using CommandListRecorders = std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>;

	GeometryPass() = default;
	~GeometryPass() = default;
	GeometryPass(const GeometryPass&) = delete;
	const GeometryPass& operator=(const GeometryPass&) = delete;
	GeometryPass(GeometryPass&&) = delete;
	GeometryPass& operator=(GeometryPass&&) = delete;

	// You should get recorders and fill them, before calling Init()
	__forceinline CommandListRecorders& GetCommandListRecorders() noexcept { return mCommandListRecorders; }

	// Preconditions:
	// - You should fill recorders with GetCommandListRecorders() before
	void Init(const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;
	
	__forceinline Microsoft::WRL::ComPtr<ID3D12Resource>* GetGeometryBuffers() noexcept { return mGeometryBuffers; }
	
	// Preconditions:
	// - Init() must be called first
	void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool IsDataValid() const noexcept;

	void ExecuteBeginTask() noexcept;

	// 1 command allocator per queued frame.	
	ID3D12CommandAllocator* mCommandAllocators[SettingsManager::sQueuedFrameCount]{ nullptr };
	ID3D12GraphicsCommandList* mCommandList{ nullptr };

	// Geometry buffers data
	Microsoft::WRL::ComPtr<ID3D12Resource> mGeometryBuffers[BUFFERS_COUNT];
	D3D12_CPU_DESCRIPTOR_HANDLE mGeometryBufferRenderTargetCpuDescriptors[BUFFERS_COUNT];

	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };
		
	CommandListRecorders mCommandListRecorders;
};
