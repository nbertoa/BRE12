#include "BasicTechScene.h"

#include <algorithm>
#include <tbb/parallel_for.h>

#include <CommandManager/CommandManager.h>
#include <GlobalData/D3dData.h>
#include <PSOCreator/Basic/BasicCmdListRecorder.h>
#include <PSOCreator/PunctualLight/PunctualLightCmdListRecorder.h>
#include <PSOCreator/PSOCreator.h>
#include <RenderTask/GeomBuffersCreator.h>
#include <ResourceManager/ResourceManager.h>

namespace {
	struct Material {
		float mBaseColor_MetalMask[4U];
		float mReflectance_Smoothness[4U];
	};

	void BuildConstantBuffers(BasicCmdListRecorder& task) noexcept {
		ASSERT(task.CbvSrvUavDescHeap() == nullptr);
		ASSERT(task.FrameCBuffer() == nullptr);
		ASSERT(task.ObjectCBuffer() == nullptr);
		ASSERT(task.MaterialsCBuffer() == nullptr);

		const std::uint32_t geomCount{ (std::uint32_t)task.GetGeometryVec().size() };
		std::uint32_t numGeomDesc{ 0U };
		for (std::size_t i = 0UL; i < geomCount; ++i) {
			numGeomDesc += (std::uint32_t)task.WorldMatricesByGeomIndex()[i].size();
		}
		ASSERT(numGeomDesc != 0U);

		// Create CBV_SRV_UAV cbuffer descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descHeapDesc.NodeMask = 0U;
		descHeapDesc.NumDescriptors = numGeomDesc * 2;
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, task.CbvSrvUavDescHeap());

		// Create materials cbuffer		
		const std::size_t matCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(Material)) };
		ResourceManager::Get().CreateUploadBuffer(matCBufferElemSize, numGeomDesc, task.MaterialsCBuffer());

		// Create object cbuffer
		const std::size_t objCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };
		ResourceManager::Get().CreateUploadBuffer(objCBufferElemSize, numGeomDesc, task.ObjectCBuffer());

		const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		D3D12_GPU_VIRTUAL_ADDRESS materialsGpuVAddress{ task.MaterialsCBuffer()->Resource()->GetGPUVirtualAddress() };
		D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ task.ObjectCBuffer()->Resource()->GetGPUVirtualAddress() };
		task.ObjectCBufferGpuDescHandleBegin() = task.CbvSrvUavDescHeap()->GetGPUDescriptorHandleForHeapStart();
		task.MaterialsCBufferGpuDescHandleBegin().ptr = task.ObjectCBufferGpuDescHandleBegin().ptr + numGeomDesc * descHandleIncSize;
		Material material;
		for (std::size_t i = 0UL; i < numGeomDesc; ++i) {
			D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvUavCpuDescHandle = task.CbvSrvUavDescHeap()->GetCPUDescriptorHandleForHeapStart();
			cbvSrvUavCpuDescHandle.ptr += i * descHandleIncSize;

			// Create CBV_SRV_UAV CBuffer descriptor
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.BufferLocation = objCBufferGpuAddress + i * objCBufferElemSize;
			cbvDesc.SizeInBytes = (std::uint32_t)objCBufferElemSize;
			ResourceManager::Get().CreateConstantBufferView(cbvDesc, cbvSrvUavCpuDescHandle);

			// Create materials CBuffer descriptor
			D3D12_CPU_DESCRIPTOR_HANDLE materialsCpuDescHandle = task.CbvSrvUavDescHeap()->GetCPUDescriptorHandleForHeapStart();
			materialsCpuDescHandle.ptr += (i + numGeomDesc) * descHandleIncSize;
			cbvDesc.BufferLocation = materialsGpuVAddress + i * matCBufferElemSize;
			cbvDesc.SizeInBytes = (std::uint32_t)matCBufferElemSize;
			ResourceManager::Get().CreateConstantBufferView(cbvDesc, materialsCpuDescHandle);

			// Fill materials cbuffer data
			material.mBaseColor_MetalMask[0] = MathHelper::RandF(0.0f, 1.0f);
			material.mBaseColor_MetalMask[1] = MathHelper::RandF(0.0f, 1.0f);
			material.mBaseColor_MetalMask[2] = MathHelper::RandF(0.0f, 1.0f);
			material.mBaseColor_MetalMask[3] = MathHelper::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[0] = MathHelper::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[1] = MathHelper::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[2] = MathHelper::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[3] = MathHelper::RandF(0.0f, 1.0f);
			task.MaterialsCBuffer()->CopyData((std::uint32_t)i, &material, sizeof(material));
		}

		// Create frame constant buffer
		const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4) * 2UL) };
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, task.FrameCBuffer());

		// Fill objects cbuffer data
		std::uint32_t k = 0U;
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

void BasicTechScene::GenerateGeomPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept {
	ASSERT(tasks.empty());

	GeometryGenerator::MeshData shape;
	GeometryGenerator::CreateSphere(2.0f, 50U, 50U, shape);

	const std::size_t numTasks{ 4UL };
	const std::size_t numGeometry{ 1000UL };
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
			BasicCmdListRecorder* newTask{ new BasicCmdListRecorder(D3dData::Device(), cmdListQueue) };
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

void BasicTechScene::GenerateLightPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	const PSOCreator::Output& psoCreatorOutput(PSOCreator::CommonPSOData::GetData(PSOCreator::CommonPSOData::PUNCTUAL_LIGHT));

	tasks.resize(1U);
	std::unique_ptr<CmdListRecorder>& task{ tasks.back() };
	PunctualLightCmdListRecorder* newTask{ new PunctualLightCmdListRecorder(D3dData::Device(), cmdListQueue) };
	task.reset(newTask);
	task->PSO() = psoCreatorOutput.mPSO;
	task->RootSign() = psoCreatorOutput.mRootSign;

	// Create CBV_SRV_UAV cbuffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = geometryBuffersCount;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, task->CbvSrvUavDescHeap());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvUavCpuDescHandle = task->CbvSrvUavDescHeap()->GetCPUDescriptorHandleForHeapStart();

	for (std::uint32_t i = 0U; i < geometryBuffersCount; ++i) {
		ID3D12Resource& res{ *geometryBuffers[i].Get() };

		srvDesc.Format = res.GetDesc().Format;
		srvDesc.Texture2D.MipLevels = res.GetDesc().MipLevels;

		ResourceManager::Get().CreateShaderResourceView(res, srvDesc, cbvSrvUavCpuDescHandle);

		cbvSrvUavCpuDescHandle.ptr += descHandleIncSize;
	}

	// Create frame constant buffer
	const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4) * 2UL) };
	ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, task->FrameCBuffer());
}

