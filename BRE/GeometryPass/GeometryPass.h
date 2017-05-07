#pragma once

#include <memory>
#include <vector>

#include <CommandManager\CommandListPerFrame.h>
#include <GeometryPass\GeometryPassCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
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

	using GeometryPassCommandListRecorders = std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>;

	GeometryPass(GeometryPassCommandListRecorders& geometryPassCommandListRecorders);
	~GeometryPass() = default;
	GeometryPass(const GeometryPass&) = delete;
	const GeometryPass& operator=(const GeometryPass&) = delete;
	GeometryPass(GeometryPass&&) = delete;
	GeometryPass& operator=(GeometryPass&&) = delete;

	// Preconditions:
	// - You should fill recorders with GetCommandListRecorders() before
	void Init(const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept;
	
	__forceinline Microsoft::WRL::ComPtr<ID3D12Resource>* GetGeometryBuffers() noexcept { return mGeometryBuffers; }
	
	// Preconditions:
	// - Init() must be called first
	void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool IsDataValid() const noexcept;

	void ExecuteBeginTask() noexcept;

	CommandListPerFrame mCommandListPerFrame;

	// Geometry buffers data
	Microsoft::WRL::ComPtr<ID3D12Resource> mGeometryBuffers[BUFFERS_COUNT];
	D3D12_CPU_DESCRIPTOR_HANDLE mGeometryBufferRenderTargetViews[BUFFERS_COUNT];

	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferView{ 0UL };
		
	GeometryPassCommandListRecorders& mGeometryPassCommandListRecorders;
};
