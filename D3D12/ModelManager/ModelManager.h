#pragma once

#include <d3d12.h>
#include <memory>
#include <tbb\concurrent_hash_map.h>
#include <tbb/mutex.h>
#include <wrl.h>

#include <ModelManager/Model.h>

class ModelManager {
public:
	static ModelManager& Create() noexcept;
	static ModelManager& Get() noexcept;

	ModelManager(const ModelManager&) = delete;
	const ModelManager& operator=(const ModelManager&) = delete;

	// Returns id to get model after creation
	std::size_t LoadModel(const char* filename, Model* &model) noexcept;

	// Asserts if id does not exist
	Model& GetModel(const std::size_t id) noexcept;

	// Asserts if id is not present
	void Erase(const std::size_t id) noexcept;

	// Invalidate all ids.
	__forceinline void Clear() noexcept { mModelById.clear(); }

private:
	ModelManager() = default;

	using ModelById = tbb::concurrent_hash_map<std::size_t, std::unique_ptr<Model>>;
	ModelById mModelById;

	tbb::mutex mMutex;
};
