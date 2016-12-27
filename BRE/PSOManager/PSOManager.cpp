#include "PSOManager.h"

#include <memory>

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>
#include <Utils/NumberGeneration.h>

namespace {
	std::unique_ptr<PSOManager> gManager{ nullptr };
}

PSOManager& PSOManager::Create() noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new PSOManager());
	return *gManager.get();
}
PSOManager& PSOManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

std::size_t PSOManager::CreateGraphicsPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, ID3D12PipelineState* &pso) noexcept {
	mMutex.lock();
	CHECK_HR(DirectXManager::Device().CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
	mMutex.unlock();
	
	const std::size_t id{ NumberGeneration::IncrementalSizeT() };
	PSOById::accessor accessor;
#ifdef _DEBUG
	mPSOById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mPSOById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12PipelineState>(pso);
	accessor.release();
	
	return id;
}

ID3D12PipelineState& PSOManager::GetPSO(const std::size_t id) noexcept {
	PSOById::accessor accessor;
	mPSOById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12PipelineState* state{ accessor->second.Get() };
	accessor.release();

	return *state;
}

void PSOManager::Erase(const std::size_t id) noexcept {
	PSOById::accessor accessor;
	mPSOById.find(accessor, id);
	ASSERT(!accessor.empty());
	mPSOById.erase(accessor);
	accessor.release();
}