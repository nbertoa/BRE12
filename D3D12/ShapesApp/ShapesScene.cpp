#include "ShapesScene.h"

#include <tbb/parallel_for.h>

#include <CommandManager/CommandManager.h>
#include <GlobalData/D3dData.h>
#include <RenderTask/GeomBuffersCreator.h>
#include <RenderTask/PSOCreator.h>
#include <RenderTask/InitTask.h>
#include <RenderTask/PSOCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <ShapesApp/ShapesCmdBuilderTask.h>

namespace {
	void BuildCommandObjects(CmdBuilderTaskInput& output) noexcept {
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

	void BuildConstantBuffers(CmdBuilderTaskInput& output) noexcept {
		ASSERT(output.mCBVHeap == nullptr);
		ASSERT(output.mFrameConstants == nullptr);
		ASSERT(output.mObjectConstants == nullptr);

		const std::uint32_t geomCount{ (std::uint32_t)output.mGeomDataVec.size() };
		std::uint32_t numGeomDesc{ 0U };
		for (std::size_t i = 0UL; i < geomCount; ++i) {
			numGeomDesc += (std::uint32_t)output.mGeomDataVec[i].mWorldMats.size();
		}
		ASSERT(numGeomDesc != 0U);

		// Create constant buffers descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descHeapDesc.NodeMask = 0U;
		descHeapDesc.NumDescriptors = numGeomDesc + 1U; // +1 for frame constants
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceManager::gManager->CreateDescriptorHeap(descHeapDesc, output.mCBVHeap);
		output.mCbvBaseGpuDescHandle = output.mCBVHeap->GetGPUDescriptorHandleForHeapStart();

		const std::size_t descHandleIncSize{ ResourceManager::gManager->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };

		const std::size_t elemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };

		// Fill constant buffers descriptor heap with per objects constants buffer views
		ResourceManager::gManager->CreateUploadBuffer(elemSize, numGeomDesc, output.mObjectConstants);
		D3D12_GPU_VIRTUAL_ADDRESS cbObjGPUBaseAddress{ output.mObjectConstants->Resource()->GetGPUVirtualAddress() };
		for (std::size_t i = 0UL; i < numGeomDesc; ++i) {
			D3D12_CPU_DESCRIPTOR_HANDLE descHandle = output.mCBVHeap->GetCPUDescriptorHandleForHeapStart();
			descHandle.ptr += i * descHandleIncSize;

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.BufferLocation = cbObjGPUBaseAddress + i * elemSize;
			cbvDesc.SizeInBytes = (std::uint32_t)elemSize;

			ResourceManager::gManager->CreateConstantBufferView(cbvDesc, descHandle);
		}

		// Fill constant buffers descriptor heap with per frame constant buffer view
		ResourceManager::gManager->CreateUploadBuffer(elemSize, 1U, output.mFrameConstants);
		D3D12_GPU_VIRTUAL_ADDRESS cbFrameGPUBaseAddress{ output.mFrameConstants->Resource()->GetGPUVirtualAddress() };
		D3D12_CPU_DESCRIPTOR_HANDLE descHandle = output.mCBVHeap->GetCPUDescriptorHandleForHeapStart();
		descHandle.ptr += numGeomDesc * descHandleIncSize;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = cbFrameGPUBaseAddress;
		cbvDesc.SizeInBytes = (std::uint32_t)elemSize;
		ResourceManager::gManager->CreateConstantBufferView(cbvDesc, descHandle);
	}
}

void ShapesScene::GenerateTasks(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdBuilderTask>>& tasks) const noexcept {
	ASSERT(tasks.empty());

	GeometryGenerator::MeshData sphere{ GeometryGenerator::CreateSphere(2, 20, 20) };
	GeometryGenerator::MeshData box{ GeometryGenerator::CreateBox(2, 2, 2, 2) };

	const std::size_t numTasks{ 4UL };
	const std::size_t numGeometry{ 1000UL };
	std::vector<std::unique_ptr<InitTask>> initTasks;
	initTasks.resize(numTasks);
	tasks.resize(numTasks);

	// Create a command list 
	ID3D12GraphicsCommandList* cmdList;
	ID3D12CommandAllocator* cmdAlloc;
	CommandManager::gManager->CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc);
	CommandManager::gManager->CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc, cmdList);

	PSOCreator::Input psoCreatorInput;
	psoCreatorInput.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
	psoCreatorInput.mPSFilename = "ShapesApp/PS.cso";
	psoCreatorInput.mRootSignFilename = "ShapesApp/RS.cso";
	psoCreatorInput.mVSFilename = "ShapesApp/VS.cso";
	PSOCreator::Output psoCreatorOutput;
	PSOCreator::Execute(psoCreatorInput, psoCreatorOutput);

	GeomBuffersCreator::Input geomBuffersCreatorInput(
		sphere.mVertices.data(), 
		(std::uint32_t)sphere.mVertices.size(), 
		sizeof(GeometryGenerator::Vertex), 
		sphere.mIndices32.data(), 
		(std::uint32_t)sphere.mIndices32.size()
	);
	GeomBuffersCreator::Output geomBuffersCreatorOutput;
	GeomBuffersCreator::Execute(cmdListQueue, *cmdList, geomBuffersCreatorInput, geomBuffersCreatorOutput);
	
	const float meshSpaceOffset{ 200.0f };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, numTasks, numTasks / Settings::sCpuProcessors),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			std::unique_ptr<CmdBuilderTask>& task{ tasks[k] };
			task.reset(new ShapesCmdBuilderTask(D3dData::mDevice.Get(), Settings::sScreenViewport, Settings::sScissorRect));
			CmdBuilderTaskInput& taskInput{ task->TaskInput() };
			taskInput.mPSO = psoCreatorOutput.mPSO;
			taskInput.mRootSign = psoCreatorOutput.mRootSign;
			BuildCommandObjects(taskInput);

			taskInput.mGeomDataVec.resize(1UL);
			GeometryData& geomData{ taskInput.mGeomDataVec.back() };
			geomData.mBuffersInfo = geomBuffersCreatorOutput;
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				const float tx{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float ty{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float tz{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
				geomData.mWorldMats.push_back(world);
			}

			BuildConstantBuffers(taskInput);
		}
	}
	);
}

