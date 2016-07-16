#include "InitTask.h"

#include <CommandManager/CommandManager.h>
#include <GlobalData\Settings.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager/RootSignatureManager.h>
#include <ShaderManager/ShaderManager.h>
#include <Utils/DebugUtils.h>

bool InitTaskInput::ValidateData() const {
	return mGeomBuffersCreatorInputVec.empty() == false && mPSOCreatorInput.ValidateData();
}

void InitTask::InitCmdBuilders(tbb::concurrent_queue<ID3D12CommandList*>& /*cmdLists*/, CmdBuilderTaskInput& output) noexcept {
	ASSERT(mInput.ValidateData());
	
	output.mTopology = mInput.mPSOCreatorInput.mTopology;

	PSOCreator::Output posCreatorOutput;
	PSOCreator::Execute(mInput.mPSOCreatorInput, posCreatorOutput);
	output.mRootSign = posCreatorOutput.mRootSign;
	output.mPSO = posCreatorOutput.mPSO;

	BuildCommandObjects(output);
}

void InitTask::BuildCommandObjects(CmdBuilderTaskInput& output) noexcept {
	ASSERT(output.mCmdList == nullptr);
	const std::uint32_t allocCount{ _countof(output.mCmdAlloc) };

#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < allocCount; ++i) {
		ASSERT(output.mCmdAlloc[i] == nullptr);
	}	
#endif
	
	for (std::uint32_t i = 0U; i < allocCount; ++i) {
		CommandManager::gManager->CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, output.mCmdAlloc[i]);
	}

	CommandManager::gManager->CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *output.mCmdAlloc[0], output.mCmdList);

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	output.mCmdList->Close();
}