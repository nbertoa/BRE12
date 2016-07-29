#include "ShapesScene.h"

#include <tbb/parallel_for.h>

#include <CommandManager/CommandManager.h>
#include <GlobalData/D3dData.h>
#include <PSOCreator/Black/BlackCmdBuilderTask.h>
#include <PSOCreator/PSOCreator.h>
#include <RenderTask/GeomBuffersCreator.h>
#include <ResourceManager/ResourceManager.h>

namespace {
	void BuildConstantBuffers(CmdListRecorder& task) noexcept {
		ASSERT(task.CbvSrvUavDescHeap() == nullptr);
		ASSERT(task.FrameCBuffer() == nullptr);
		ASSERT(task.ObjectCBuffer() == nullptr);

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
		descHeapDesc.NumDescriptors = numGeomDesc;
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, task.CbvSrvUavDescHeap());

		const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };

		const std::size_t cbSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };

		// Fill constant buffers descriptor heap with per objects constants buffer views
		ResourceManager::Get().CreateUploadBuffer(cbSize, numGeomDesc, task.ObjectCBuffer());
		D3D12_GPU_VIRTUAL_ADDRESS objectsCBufferGpuAddress{ task.ObjectCBuffer()->Resource()->GetGPUVirtualAddress() };
		for (std::size_t i = 0UL; i < numGeomDesc; ++i) {
			D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvUavCpuDescHandle = task.CbvSrvUavDescHeap()->GetCPUDescriptorHandleForHeapStart();
			cbvSrvUavCpuDescHandle.ptr += i * descHandleIncSize;

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.BufferLocation = objectsCBufferGpuAddress + i * cbSize;
			cbvDesc.SizeInBytes = (std::uint32_t)cbSize;

			ResourceManager::Get().CreateConstantBufferView(cbvDesc, cbvSrvUavCpuDescHandle);
		}

		// Create upload buffer for frame constants
		ResourceManager::Get().CreateUploadBuffer(cbSize, 1U, task.FrameCBuffer());

		// Fill object cbuffer data
		std::uint32_t k{ 0U };
		for (std::size_t i = 0UL; i < geomCount; ++i) {
			const std::uint32_t worldMatsCount{ (std::uint32_t)task.WorldMatricesByGeomIndex()[i].size() };
			for (std::uint32_t j = 0UL; j < worldMatsCount; ++j) {
				DirectX::XMFLOAT4X4 w;
				const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&task.WorldMatricesByGeomIndex()[i][j]));
				DirectX::XMStoreFloat4x4(&w, wMatrix);
				task.ObjectCBuffer()->CopyData(k + j, &w, sizeof(w));
			}
			k += worldMatsCount;
		}
	}
}

void ShapesScene::GenerateCmdListRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept {
	ASSERT(tasks.empty());

	GeometryGenerator::MeshData sphere;
	GeometryGenerator::CreateSphere(2.0f, 20U, 20U, sphere);
	GeometryGenerator::MeshData box;
	GeometryGenerator::CreateBox(2.0f, 2.0f, 2.0f, 2U, box);

	const std::size_t numTasks{ 4UL };
	const std::size_t numGeometry{ 1000UL };
	tasks.resize(numTasks);

	// Create a command list 
	ID3D12GraphicsCommandList* cmdList;
	ID3D12CommandAllocator* cmdAlloc;
	CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc);
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc, cmdList);

	const PSOCreator::Output& psoCreatorOutput(PSOCreator::CommonPSOData::GetData(PSOCreator::CommonPSOData::BLACK));

	GeomBuffersCreator::Input geomBuffersCreatorInput(
		sphere.mVertices.data(), 
		(std::uint32_t)sphere.mVertices.size(), 
		sizeof(GeometryGenerator::Vertex), 
		sphere.mIndices32.data(), 
		(std::uint32_t)sphere.mIndices32.size()
	);
	GeomBuffersCreator::Output geomBuffersCreatorOutput;
	GeomBuffersCreator::Execute(cmdListQueue, *cmdList, geomBuffersCreatorInput, geomBuffersCreatorOutput);
	
	const float meshSpaceOffset{ 100.0f };
	const std::uint32_t grainSize{ max(1U, (std::uint32_t)numTasks / Settings::sCpuProcessors) };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, numTasks, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			std::unique_ptr<CmdListRecorder>& task{ tasks[k] };
			task.reset(new BlackCmdBuilderTask(D3dData::Device(), cmdListQueue));
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

			BuildConstantBuffers(*task.get());
		}
	}
	);
}

