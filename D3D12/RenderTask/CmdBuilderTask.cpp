#include "CmdBuilderTask.h"

#include <Utils/DebugUtils.h>

CmdBuilderTask::CmdBuilderTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect, CmdBuilderTaskInput& input)
	: mDevice(device)
	, mViewport(screenViewport)
	, mScissorRect(scissorRect)
	, mInput(input)
{
	ASSERT(device != nullptr);
}