#pragma once

#include <cstdint>
#include <d3d12.h>
#include <DirectXMath.h>
#include <tbb/concurrent_queue.h>
#include <wrl.h>

#include <DXUtils/D3DFactory.h>
#include <GeometryGenerator/GeometryGenerator.h>
#include <MathUtils/MathHelper.h>
#include <RenderTask/CmdBuilderTask.h>
#include <RenderTask\PSOCreator.h>
#include <RenderTask/GeomBuffersCreator.h>
#include <Utils/DebugUtils.h>

class UploadBuffer;

struct InitTaskInput {
	InitTaskInput() = default;

	bool ValidateData() const;

	using GeomBuffersCreatorInputVec = std::vector<GeomBuffersCreator::Input>;
	GeomBuffersCreatorInputVec mGeomBuffersCreatorInputVec{};
	std::vector<DirectX::XMFLOAT4X4> mWorldVec;

	PSOCreator::Input mPSOCreatorInput;
};

// Task that given its InitTaskInput, initializes a CmdBuilderTaskOutput (that will be used to build command lists)
// Steps:
// - Inherit from InitTask and reimplement InitCmdBuilders() method (you should call InitTask::InitCmdBuilders() method in reimplementation)
// - Fill InitTaskInput data (InitTask::TaskInput())
// - Call InitTask::InitCmdBuilders() to fill CmdBuilderTaskInput accordingly.
class InitTask {
public:
	InitTask() = default;

	__forceinline InitTaskInput& TaskInput() noexcept { return mInput; }

	// We require to pass the queue of command lists for initialization purposes (create resources, buffers, etc)
	virtual void InitCmdBuilders(tbb::concurrent_queue<ID3D12CommandList*>& cmdLists, CmdBuilderTaskInput& output) noexcept;

protected:
	void BuildCommandObjects(CmdBuilderTaskInput& output) noexcept;

	InitTaskInput mInput{};
};