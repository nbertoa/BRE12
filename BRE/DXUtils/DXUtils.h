#pragma once

struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12GraphicsCommandList;

namespace DXUtils {
	// Preconditions:
	// - Command list must not be closed
	void ExecuteCommandListAndWaitForCompletion(
		ID3D12CommandQueue& cmdQueue,
		ID3D12GraphicsCommandList& cmdList,
		ID3D12Fence& fence) noexcept;
}
