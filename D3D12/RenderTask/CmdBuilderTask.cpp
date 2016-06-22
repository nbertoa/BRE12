#include "CmdBuilderTask.h"

#include <Utils/DebugUtils.h>

CmdBuilderTask::CmdBuilderTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect)
	: mDevice(device)
	, mViewport(screenViewport)
	, mScissorRect(scissorRect)
{
	ASSERT(device != nullptr);
}
