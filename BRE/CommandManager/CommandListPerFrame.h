#pragma once

#include <cstdint>

#include <SettingsManager\SettingsManager.h>
#include <Utils\DebugUtils.h>

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12PipelinesState;

// We support to have different number of queued frames.
// This class provides a command list that can be reset
// with a different command allocator per queued frame.
class CommandListPerFrame {
public:
	CommandListPerFrame();
	~CommandListPerFrame() = default;
	CommandListPerFrame(const CommandListPerFrame&) = delete;
	const CommandListPerFrame& operator=(const CommandListPerFrame&) = delete;
	CommandListPerFrame(CommandListPerFrame&&) = default;
	CommandListPerFrame& operator=(CommandListPerFrame&&) = default;

	ID3D12GraphicsCommandList& ResetWithNextCommandAllocator(ID3D12PipelineState* pso) noexcept;
	__forceinline ID3D12GraphicsCommandList& GetCommandList() noexcept {
		ASSERT(mCommandList != nullptr);
		return *mCommandList; 
	}

private:
	ID3D12CommandAllocator* mCommandAllocators[SettingsManager::sQueuedFrameCount]{ nullptr };
	ID3D12GraphicsCommandList* mCommandList{ nullptr };
	std::uint32_t mCurrentFrameIndex{ 0U };
};
