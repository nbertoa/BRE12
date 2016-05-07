#include "PSOManager.h"

#include <Utils/DebugUtils.h>

std::unique_ptr<PSOManager> PSOManager::gPSOMgr = nullptr;

size_t PSOManager::CreateGraphicsPSO(const std::string& name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pso) {
	ASSERT(!name.empty());

	const size_t id = mHash(name);
	PSOById::iterator it = mPSOById.find(id);
	if (it != mPSOById.end()) {
		pso = it->second;
	}
	else {
		CHECK_HR(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		mPSOById.insert(IdAndPSO(id, pso));
	}
	
	return id;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PSOManager::GetPSO(const size_t id) {
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	PSOById::iterator it = mPSOById.find(id);
	ASSERT(it != mPSOById.end());

	return it->second;
}

void PSOManager::Erase(const size_t id) {
	ASSERT(mPSOById.find(id) != mPSOById.end());
	mPSOById.erase(id);
}