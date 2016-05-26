#include "PSOManager.h"

#include <Utils/DebugUtils.h>
#include <Utils/HashUtils.h>

std::unique_ptr<PSOManager> PSOManager::gManager = nullptr;

std::size_t PSOManager::CreateGraphicsPSO(const char* name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, ID3D12PipelineState* &pso) noexcept {
	ASSERT(name != nullptr);

	const std::size_t id{ HashUtils::HashCString(name) };
	PSOById::accessor accessor;
	mPSOById.find(accessor, id);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> auxPso;
	if (!accessor.empty()) {
		auxPso = accessor->second.Get();
	}
	else {
		CHECK_HR(mDevice.CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&auxPso)));
		mPSOById.insert(accessor, id);
		accessor->second = auxPso;
	}
	accessor.release();

	pso = auxPso.Get();
	
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