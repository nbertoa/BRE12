#pragma once

#include <memory>

#include <SkyBoxPass\SkyBoxCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;

class SkyBoxPass {
public:
	using CommandListRecorders = std::unique_ptr<SkyBoxCmdListRecorder>;

	SkyBoxPass() = default;
	~SkyBoxPass() = default;
	SkyBoxPass(const SkyBoxPass&) = delete;
	const SkyBoxPass& operator=(const SkyBoxPass&) = delete;
	SkyBoxPass(SkyBoxPass&&) = delete;
	SkyBoxPass& operator=(SkyBoxPass&&) = delete;

	void Init(
		ID3D12Resource& skyBoxCubeMap,
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	// Preconditions:
	// - Init() must be called first
	void Execute(const FrameCBuffer& frameCBuffer) const noexcept;

private:
	// Method used internally for validation purposes
	bool IsDataValid() const noexcept;
	
	ID3D12CommandAllocator* mCommandAllocators{ nullptr };
	ID3D12GraphicsCommandList* mCommandList{ nullptr };

	ID3D12Fence* mFence{ nullptr };

	CommandListRecorders mRecorder;
};
