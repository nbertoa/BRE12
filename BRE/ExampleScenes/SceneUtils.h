#pragma once

#include <vector>

struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

namespace SceneUtils {
	// This method load all resources from texFiles and store them in textures parameters,
	// in the same order of texFiles
	void LoadTextures(
		const std::vector<std::string>& texFiles,
		ID3D12CommandQueue& cmdQueue, 
		ID3D12CommandAllocator& cmdAlloc, 
		ID3D12GraphicsCommandList& cmdList, 
		ID3D12Fence& fence,
		std::vector<ID3D12Resource*>& textures) noexcept;

};
