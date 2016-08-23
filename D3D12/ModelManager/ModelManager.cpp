#include "ModelManager.h"

#include <cstdint>

#include <Utils/DebugUtils.h>
#include <Utils\NumberGeneration.h>

namespace {

	std::unique_ptr<ModelManager> gManager{ nullptr };
}

ModelManager& ModelManager::Create() noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new ModelManager());
	return *gManager.get();
}
ModelManager& ModelManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

std::size_t ModelManager::LoadModel(const char* filename, Model* &model) noexcept {
	ASSERT(filename != nullptr);

	mMutex.lock();
	model = new Model(filename);
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::IncrementalSizeT() };
	ModelById::accessor accessor;
#ifdef _DEBUG
	mModelById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mModelById.insert(accessor, id);
	accessor->second.reset(model);
	accessor.release();

	return id;
}

Model& ModelManager::GetModel(const std::size_t id) noexcept {
	ModelById::accessor accessor;
	mModelById.find(accessor, id);
	ASSERT(!accessor.empty());

	Model* model{ accessor->second.get() };
	accessor.release();

	return *model;
}


void ModelManager::Erase(const std::size_t id) noexcept {
	ModelById::accessor accessor;
	mModelById.find(accessor, id);
	ASSERT(!accessor.empty());
	mModelById.erase(accessor);
	accessor.release();
}