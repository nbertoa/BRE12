#include "ShapesScene.h"

#include <tbb/parallel_for.h>

#include <CommandManager/CommandManager.h>
#include <GlobalData/D3dData.h>
#include <RenderTask/GeomBuffersCreator.h>
#include <RenderTask/PSOCreator.h>
#include <RenderTask/PSOCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <ShapesApp/ShapesCmdBuilderTask.h>

namespace {
	void BuildConstantBuffers(CmdBuilderTask& task) noexcept {
		ASSERT(task.CVBHeap() == nullptr);
		ASSERT(task.FrameConstants() == nullptr);
		ASSERT(task.ObjectConstants() == nullptr);

		const std::uint32_t geomCount{ (std::uint32_t)task.GetGeometryVec().size() };
		std::uint32_t numGeomDesc{ 0U };
		for (std::size_t i = 0UL; i < geomCount; ++i) {
			numGeomDesc += (std::uint32_t)task.WorldMatricesByGeomIndex()[i].size();
		}
		ASSERT(numGeomDesc != 0U);

		// Create constant buffers descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descHeapDesc.NodeMask = 0U;
		descHeapDesc.NumDescriptors = numGeomDesc + 1U; // +1 for frame constants
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceManager::gManager->CreateDescriptorHeap(descHeapDesc, task.CVBHeap());
		task.CBVBaseGpuDescHandle() = task.CVBHeap()->GetGPUDescriptorHandleForHeapStart();

		const std::size_t descHandleIncSize{ ResourceManager::gManager->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };

		const std::size_t elemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };

		// Fill constant buffers descriptor heap with per objects constants buffer views
		ResourceManager::gManager->CreateUploadBuffer(elemSize, numGeomDesc, task.ObjectConstants());
		D3D12_GPU_VIRTUAL_ADDRESS cbObjGPUBaseAddress{ task.ObjectConstants()->Resource()->GetGPUVirtualAddress() };
		for (std::size_t i = 0UL; i < numGeomDesc; ++i) {
			D3D12_CPU_DESCRIPTOR_HANDLE descHandle = task.CVBHeap()->GetCPUDescriptorHandleForHeapStart();
			descHandle.ptr += i * descHandleIncSize;

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.BufferLocation = cbObjGPUBaseAddress + i * elemSize;
			cbvDesc.SizeInBytes = (std::uint32_t)elemSize;

			ResourceManager::gManager->CreateConstantBufferView(cbvDesc, descHandle);
		}

		// Fill constant buffers descriptor heap with per frame constant buffer view
		ResourceManager::gManager->CreateUploadBuffer(elemSize, 1U, task.FrameConstants());
		D3D12_GPU_VIRTUAL_ADDRESS cbFrameGPUBaseAddress{ task.FrameConstants()->Resource()->GetGPUVirtualAddress() };
		D3D12_CPU_DESCRIPTOR_HANDLE descHandle = task.CVBHeap()->GetCPUDescriptorHandleForHeapStart();
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
			task.reset(new ShapesCmdBuilderTask(*D3dData::mDevice.Get(), cmdListQueue));
			task->PSO() = psoCreatorOutput.mPSO;
			task->RootSign() = psoCreatorOutput.mRootSign;

			task->GetGeometryVec().resize(1UL, geomBuffersCreatorOutput);
			CmdBuilderTask::MatricesByGeomIndex& worldMatByGeomIndex{ task->WorldMatricesByGeomIndex() };
			worldMatByGeomIndex.resize(1UL, CmdBuilderTask::Matrices(numGeometry));
			for (std::size_t i = 0UL; i < numGeometry; ++i) {				
				const float tx{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float ty{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float tz{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
				worldMatByGeomIndex[0][i] = world;
			}

			BuildConstantBuffers(*task.get());
		}
	}
	);
}

