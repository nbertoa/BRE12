#pragma once

#include <cstddef>
#include <vector>

struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
class Model;

namespace SceneUtils {

	class ResourceContainer {
	public:
		ResourceContainer() = default;
		ResourceContainer(const ResourceContainer&) = delete;
		const ResourceContainer& operator=(const ResourceContainer&) = delete;

		// Load all textures from texFiles. Texture index will be equal
		// to its index in texFiles vector.
		// Precondition: You must call this method at most once.
		// Subsequent calls will fail.
		void LoadTextures(
			const std::vector<std::string>& texFiles,
			ID3D12CommandQueue& cmdQueue,
			ID3D12CommandAllocator& cmdAlloc,
			ID3D12GraphicsCommandList& cmdList,
			ID3D12Fence& fence) noexcept;

		ID3D12Resource& GetResource(const std::size_t index) noexcept;
		std::vector<ID3D12Resource*>& GetResources() noexcept { return mTextures; }

		// Load all models from modelFiles. Model index will be equal
		// to its index in modelFiles vector.
		// Precondition: You must call this method at most once.
		// Subsequent calls will fail.
		void LoadModels(
			const std::vector<std::string>& modelFiles,
			ID3D12CommandQueue& cmdQueue,
			ID3D12CommandAllocator& cmdAlloc,
			ID3D12GraphicsCommandList& cmdList,
			ID3D12Fence& fence) noexcept;

		Model& GetModel(const std::size_t index) noexcept;
		std::vector<Model*>& GetModels() noexcept { return mModels; }

	private:
		std::vector<ID3D12Resource*> mTextures;
		std::vector<Model*> mModels;
	};
};
