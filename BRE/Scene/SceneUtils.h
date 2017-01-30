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

	class SceneResources {
	public:
		SceneResources() = default;
		~SceneResources() = default;
		SceneResources(const SceneResources&) = delete;
		const SceneResources& operator=(const SceneResources&) = delete;
		SceneResources(SceneResources&&) = delete;
		SceneResources& operator=(SceneResources&&) = delete;

		void LoadTextures(
			const std::vector<std::string>& sourceTextureFiles,
			ID3D12CommandAllocator& cmdAlloc,
			ID3D12GraphicsCommandList& cmdList,
			ID3D12Fence& fence) noexcept;

		// Preconditionts:
		// - There must be a valid texture at "index" 
		ID3D12Resource& GetTexture(const std::size_t index) noexcept;
		const std::vector<ID3D12Resource*>& GetTextures() const noexcept { return mTextures; }

		void LoadModels(
			const std::vector<std::string>& modelFilenames,
			ID3D12CommandAllocator& cmdAlloc,
			ID3D12GraphicsCommandList& cmdList,
			ID3D12Fence& fence) noexcept;

		// Preconditionts:
		// - There must be a valid model at "index" 
		const Model& GetModel(const std::size_t index) const noexcept;		
		const std::vector<Model*>& GetModels() const noexcept { return mModels; }

	private:
		std::vector<ID3D12Resource*> mTextures;
		std::vector<Model*> mModels;
	};
};
