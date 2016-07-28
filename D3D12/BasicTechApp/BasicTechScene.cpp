#include "BasicTechScene.h"

#include <algorithm>
#include <tbb/parallel_for.h>

#include <BasicTechApp/BasicTechCmdBuilderTask.h>
#include <CommandManager/CommandManager.h>
#include <GlobalData/D3dData.h>
#include <PSOCreator/PSOCreator.h>
#include <RenderTask/GeomBuffersCreator.h>
#include <ResourceManager/ResourceManager.h>

namespace {
	struct Material {
		float mBaseColor_MetalMask[4U];
		float mReflectance_Smoothness[4U];
	};

	void BuildConstantBuffers(BasicTechCmdBuilderTask& task) noexcept {
		ASSERT(task.CVBHeap() == nullptr);
		ASSERT(task.FrameConstants() == nullptr);
		ASSERT(task.ObjectConstants() == nullptr);
		ASSERT(task.MaterialsBuffer() == nullptr);

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
		descHeapDesc.NumDescriptors = numGeomDesc * 2;
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, task.CVBHeap());

		const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };

		std::size_t elemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };

		// Fill constant buffers descriptor heap with per objects constants buffer views
		Material material;
		material.mBaseColor_MetalMask[0] = 1.0f;
		material.mBaseColor_MetalMask[1] = 0.71f;
		material.mBaseColor_MetalMask[2] = 0.29f;
		material.mBaseColor_MetalMask[3] = 0.0f;
		material.mReflectance_Smoothness[0] = 0.1f;
		material.mReflectance_Smoothness[1] = 0.1f;
		material.mReflectance_Smoothness[2] = 0.1f;
		material.mReflectance_Smoothness[3] = 0.6f;
		const std::size_t matSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(Material)) };
		ResourceManager::Get().CreateUploadBuffer(matSize, numGeomDesc, task.MaterialsBuffer());
		D3D12_GPU_VIRTUAL_ADDRESS materialsGpuVAddress{ task.MaterialsBuffer()->Resource()->GetGPUVirtualAddress() };

		ResourceManager::Get().CreateUploadBuffer(elemSize, numGeomDesc, task.ObjectConstants());
		D3D12_GPU_VIRTUAL_ADDRESS cbObjGPUBaseAddress{ task.ObjectConstants()->Resource()->GetGPUVirtualAddress() };

		task.MaterialsGpuDescHandleBegin().ptr = task.CVBHeap()->GetGPUDescriptorHandleForHeapStart().ptr + numGeomDesc * descHandleIncSize;

		for (std::size_t i = 0UL; i < numGeomDesc; ++i) {
			D3D12_CPU_DESCRIPTOR_HANDLE descHandle = task.CVBHeap()->GetCPUDescriptorHandleForHeapStart();
			descHandle.ptr += i * descHandleIncSize;

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.BufferLocation = cbObjGPUBaseAddress + i * elemSize;
			cbvDesc.SizeInBytes = (std::uint32_t)elemSize;
			ResourceManager::Get().CreateConstantBufferView(cbvDesc, descHandle);

			D3D12_CPU_DESCRIPTOR_HANDLE materialsCpuDescHandle = task.CVBHeap()->GetCPUDescriptorHandleForHeapStart();
			materialsCpuDescHandle.ptr += (i + numGeomDesc) * descHandleIncSize;
			cbvDesc.BufferLocation = materialsGpuVAddress + i * matSize;
			cbvDesc.SizeInBytes = (std::uint32_t)matSize;
			ResourceManager::Get().CreateConstantBufferView(cbvDesc, materialsCpuDescHandle);

			material.mBaseColor_MetalMask[0] = MathHelper::RandF(0.0f, 1.0f);
			material.mBaseColor_MetalMask[1] = MathHelper::RandF(0.0f, 1.0f);
			material.mBaseColor_MetalMask[2] = MathHelper::RandF(0.0f, 1.0f);
			material.mBaseColor_MetalMask[3] = MathHelper::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[0] = MathHelper::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[1] = MathHelper::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[2] = MathHelper::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[3] = MathHelper::RandF(0.0f, 1.0f);

			task.MaterialsBuffer()->CopyData((std::uint32_t)i, &material, sizeof(material));
		}

		// Create upload buffer for frame constants
		elemSize = UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4) * 2UL);
		ResourceManager::Get().CreateUploadBuffer(elemSize, 1U, task.FrameConstants());		
	}
}

void BasicTechScene::GenerateCmdListRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept {
	ASSERT(tasks.empty());

	GeometryGenerator::MeshData shape;
	GeometryGenerator::CreateSphere(2.0f, 50U, 50U, shape);

	const std::size_t numTasks{ 4UL };
	const std::size_t numGeometry{ 2500UL };
	tasks.resize(numTasks);

	// Create a command list 
	ID3D12GraphicsCommandList* cmdList;
	ID3D12CommandAllocator* cmdAlloc;
	CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc);
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc, cmdList);

	const PSOCreator::Output& psoCreatorOutput(PSOCreator::CommonPSOData::GetData(PSOCreator::CommonPSOData::BASIC));

	GeomBuffersCreator::Input geomBuffersCreatorInput(
		shape.mVertices.data(),
		(std::uint32_t)shape.mVertices.size(),
		sizeof(GeometryGenerator::Vertex), 
		shape.mIndices32.data(),
		(std::uint32_t)shape.mIndices32.size()
	);
	GeomBuffersCreator::Output geomBuffersCreatorOutput;
	GeomBuffersCreator::Execute(cmdListQueue, *cmdList, geomBuffersCreatorInput, geomBuffersCreatorOutput);
	
	const float meshSpaceOffset{ 50.0f };
	const std::uint32_t grainSize{ max(1U, (std::uint32_t)numTasks / Settings::sCpuProcessors) };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, numTasks, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			std::unique_ptr<CmdListRecorder>& task{ tasks[k] };
			BasicTechCmdBuilderTask* newTask{ new BasicTechCmdBuilderTask(D3dData::Device(), cmdListQueue) };
			task.reset(newTask);
			task->PSO() = psoCreatorOutput.mPSO;
			task->RootSign() = psoCreatorOutput.mRootSign;

			task->GetGeometryVec().resize(1UL, geomBuffersCreatorOutput);
			CmdListRecorder::MatricesByGeomIndex& worldMatByGeomIndex{ task->WorldMatricesByGeomIndex() };
			worldMatByGeomIndex.resize(1UL, CmdListRecorder::Matrices(numGeometry));
			for (std::size_t i = 0UL; i < numGeometry; ++i) {				
				const float tx{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float ty{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float tz{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
				worldMatByGeomIndex[0][i] = world;
			}

			BuildConstantBuffers(*newTask);
		}
	}
	);
}
