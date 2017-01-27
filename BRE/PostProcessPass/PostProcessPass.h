#pragma once

#include <memory>

#include <PostProcessPass\PostProcessCmdListRecorder.h>
#include <SettingsManager\SettingsManager.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Pass that applies post processing effects (anti aliasing, color grading, etc)
class PostProcessPass {
public:
	using Recorder = std::unique_ptr<PostProcessCmdListRecorder>;

	PostProcessPass() = default;
	~PostProcessPass() = default;
	PostProcessPass(const PostProcessPass&) = delete;
	const PostProcessPass& operator=(const PostProcessPass&) = delete;
	PostProcessPass(PostProcessPass&&) = delete;
	PostProcessPass& operator=(PostProcessPass&&) = delete;

	// You should call this method before Execute()
	void Init(
		ID3D12CommandQueue& cmdQueue,
		ID3D12Resource& colorBuffer) noexcept;

	void Execute(
		ID3D12Resource& frameBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept;

private:
	// Method used internally for validation purposes
	bool IsDataValid() const noexcept;

	void ExecuteBeginTask(
		ID3D12Resource& frameBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept;

	ID3D12CommandQueue* mCmdQueue{ nullptr };

	// 1 command allocater per queued frame.	
	ID3D12CommandAllocator* mCmdAllocs[SettingsManager::sQueuedFrameCount]{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	ID3D12Fence* mFence{ nullptr };
	
	ID3D12Resource* mColorBuffer{ nullptr };

	Recorder mRecorder;
};
