#include "BasicScene.h"

#include <algorithm>
#include <tbb/parallel_for.h>

#include <CommandManager/CommandManager.h>
#include <GeometryGenerator/GeometryGenerator.h>
#include <GlobalData/D3dData.h>
#include <PSOCreator/PSOCreator.h>
#include <ResourceManager/BufferCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <Scene/CmdListRecorders/BasicCmdListRecorder.h>
#include <Scene/CmdListRecorders/PunctualLightCmdListRecorder.h>

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

		const std::uint32_t geomCount{ (std::uint32_t)task.GetVertexAndIndexBufferDataVec().size() };
		std::uint32_t numGeomDesc{ 0U };
		for (std::size_t i = 0UL; i < geomCount; ++i) {
			numGeomDesc += (std::uint32_t)task.WorldMatricesByGeomIndex()[i].size();
		}
		ASSERT(numGeomDesc != 0U);

		// Create CBV_SRV_UAV cbuffer descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descHeapDesc.NodeMask = 0U;
		descHeapDesc.NumDescriptors = numGeomDesc * 2; // 1 obj cbuffer + 1 material cbuffer per geometry to draw
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, task.CbvSrvUavDescHeap());
		
		// Create object cbuffer
		const std::size_t objCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };
		ResourceManager::Get().CreateUploadBuffer(objCBufferElemSize, numGeomDesc, task.ObjectCBuffer());
		task.ObjectCBufferGpuDescHandleBegin() = task.CbvSrvUavDescHeap()->GetGPUDescriptorHandleForHeapStart();

		// Create materials cbuffer		
		const std::size_t matCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(Material)) };
		ResourceManager::Get().CreateUploadBuffer(matCBufferElemSize, numGeomDesc, task.MaterialsCBuffer());
		const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		task.MaterialsCBufferGpuDescHandleBegin().ptr = task.ObjectCBufferGpuDescHandleBegin().ptr + numGeomDesc * descHandleIncSize;

		// Create object cbuffer descriptors
		// Create material cbuffer descriptors
		// Fill materials cbuffers data
		D3D12_GPU_VIRTUAL_ADDRESS materialsGpuAddress{ task.MaterialsCBuffer()->Resource()->GetGPUVirtualAddress() };
		D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ task.ObjectCBuffer()->Resource()->GetGPUVirtualAddress() };
		Material material;
		D3D12_CPU_DESCRIPTOR_HANDLE currObjCBufferDescHandle(task.CbvSrvUavDescHeap()->GetCPUDescriptorHandleForHeapStart());
		D3D12_CPU_DESCRIPTOR_HANDLE currMaterialCBufferDescHandle{ task.CbvSrvUavDescHeap()->GetCPUDescriptorHandleForHeapStart().ptr + numGeomDesc * descHandleIncSize };
		for (std::size_t i = 0UL; i < numGeomDesc; ++i) {
			// Create object cbuffers descriptors
			D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferDesc{};
			cBufferDesc.BufferLocation = objCBufferGpuAddress + i * objCBufferElemSize;
			cBufferDesc.SizeInBytes = (std::uint32_t)objCBufferElemSize;
			ResourceManager::Get().CreateConstantBufferView(cBufferDesc, currObjCBufferDescHandle);

			// Create materials CBuffer descriptor
			cBufferDesc.BufferLocation = materialsGpuAddress + i * matCBufferElemSize;
			cBufferDesc.SizeInBytes = (std::uint32_t)matCBufferElemSize;
			ResourceManager::Get().CreateConstantBufferView(cBufferDesc, currMaterialCBufferDescHandle);

			// Fill materials cbuffer data
			material.mBaseColor_MetalMask[0] = MathUtils::RandF(0.0f, 1.0f);
			material.mBaseColor_MetalMask[1] = MathUtils::RandF(0.0f, 1.0f);
			material.mBaseColor_MetalMask[2] = MathUtils::RandF(0.0f, 1.0f);
			material.mBaseColor_MetalMask[3] = MathUtils::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[0] = MathUtils::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[1] = MathUtils::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[2] = MathUtils::RandF(0.0f, 1.0f);
			material.mReflectance_Smoothness[3] = MathUtils::RandF(0.0f, 1.0f);
			task.MaterialsCBuffer()->CopyData((std::uint32_t)i, &material, sizeof(material));

			currMaterialCBufferDescHandle.ptr += descHandleIncSize;
			currObjCBufferDescHandle.ptr += descHandleIncSize;
		}

		// Create frame cbuffer
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

	void BuildConstantBuffers(PunctualLightCmdListRecorder& task, const std::uint32_t geometryBuffersCount) noexcept {
		ASSERT(task.CbvSrvUavDescHeap() == nullptr);
		ASSERT(task.FrameCBuffer() == nullptr);
		ASSERT(task.ObjectCBuffer() == nullptr);

		// We assume we have world matrices by geometry index 0 (but we do not have geometry here)
		ASSERT(task.WorldMatricesByGeomIndex().size() == 1UL);
		ASSERT(task.WorldMatricesByGeomIndex()[0].empty() == false);
		const std::uint32_t lightsCount{ (std::uint32_t)task.WorldMatricesByGeomIndex()[0].size() };
		const std::uint32_t numGeomDesc{ lightsCount };

		// Create CBV_SRV_UAV cbuffer descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descHeapDesc.NodeMask = 0U;
		descHeapDesc.NumDescriptors = numGeomDesc + geometryBuffersCount;
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, task.CbvSrvUavDescHeap());

		// Create object cbuffer
		const std::size_t objCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };
		ResourceManager::Get().CreateUploadBuffer(objCBufferElemSize, numGeomDesc, task.ObjectCBuffer());
		const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		task.ObjectCBufferGpuDescHandleBegin().ptr = task.CbvSrvUavDescHeap()->GetGPUDescriptorHandleForHeapStart().ptr + geometryBuffersCount * descHandleIncSize;
		
		// Create object cbuffer descriptors
		// Fill materials cbuffers data		
		D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ task.ObjectCBuffer()->Resource()->GetGPUVirtualAddress() };
		D3D12_CPU_DESCRIPTOR_HANDLE currObjCBufferDescHandle{ task.CbvSrvUavDescHeap()->GetCPUDescriptorHandleForHeapStart().ptr + geometryBuffersCount * descHandleIncSize };
		for (std::size_t i = 0UL; i < numGeomDesc; ++i) {
			// Create object cbuffers descriptors
			D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferDesc{};
			cBufferDesc.BufferLocation = objCBufferGpuAddress + i * objCBufferElemSize;
			cBufferDesc.SizeInBytes = (std::uint32_t)objCBufferElemSize;
			ResourceManager::Get().CreateConstantBufferView(cBufferDesc, currObjCBufferDescHandle);

			currObjCBufferDescHandle.ptr += descHandleIncSize;
		}

		// Create frame cbuffer
		const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4) * 2UL) };
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, task.FrameCBuffer());

		// Fill objects cbuffer data
		for (std::uint32_t j = 0UL; j < lightsCount; ++j) {
			DirectX::XMFLOAT4X4 w;
			const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&task.WorldMatricesByGeomIndex()[0][j]));
			DirectX::XMStoreFloat4x4(&w, wMatrix);
			task.ObjectCBuffer()->CopyData(j, &w, sizeof(w));
		}
	}
}

void BasicScene::GenerateGeomPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept {
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

	// Create vertex buffer
	BufferCreator::BufferParams vertexBufferParams(shape.mVertices.data(), (std::uint32_t)shape.mVertices.size(), sizeof(GeometryGenerator::Vertex));
	BufferCreator::VertexBufferData vertexBufferData;
	BufferCreator::CreateBuffer(*cmdList, vertexBufferParams, vertexBufferData);

	// Create index buffer
	BufferCreator::BufferParams indexBufferParams(shape.mIndices32.data(), (std::uint32_t)shape.mIndices32.size(), sizeof(std::uint32_t));
	BufferCreator::IndexBufferData indexBufferData;
	BufferCreator::CreateBuffer(*cmdList, indexBufferParams, indexBufferData);

	cmdList->Close();
	cmdListQueue.push(cmdList);

	CmdListRecorder::VertexAndIndexBufferData vAndIData(std::make_pair(vertexBufferData, indexBufferData));

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
						
			task->GetVertexAndIndexBufferDataVec().resize(1UL, vAndIData);
			CmdListRecorder::MatricesByGeomIndex& worldMatByGeomIndex{ task->WorldMatricesByGeomIndex() };
			worldMatByGeomIndex.resize(1UL, CmdListRecorder::Matrices(numGeometry));
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				const float tx{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float ty{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float tz{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
				worldMatByGeomIndex[0][i] = world;
			}

			BuildConstantBuffers(*newTask);
		}
	}
	);
}

void BasicScene::GenerateLightPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	const PSOCreator::Output& psoCreatorOutput(PSOCreator::CommonPSOData::GetData(PSOCreator::CommonPSOData::PUNCTUAL_LIGHT));

	const std::size_t numTasks{ 1UL };
	const std::size_t numLights{ 2UL };
	tasks.resize(numTasks);

	const float lightSpaceOffset{ 0.1f };
	const std::uint32_t grainSize{ max(1U, (std::uint32_t)numTasks / Settings::sCpuProcessors) };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, numTasks, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			std::unique_ptr<CmdListRecorder>& task{ tasks[k] };
			PunctualLightCmdListRecorder* newTask{ new PunctualLightCmdListRecorder(D3dData::Device(), cmdListQueue) };
			task.reset(newTask);
			task->PSO() = psoCreatorOutput.mPSO;
			task->RootSign() = psoCreatorOutput.mRootSign;

			CmdListRecorder::MatricesByGeomIndex& worldMatByGeomIndex{ task->WorldMatricesByGeomIndex() };
			task->GetVertexAndIndexBufferDataVec().resize(1UL);
			worldMatByGeomIndex.resize(1UL, CmdListRecorder::Matrices(numLights));
			for (std::size_t i = 0UL; i < numLights; ++i) {
				const float tx{ MathUtils::RandF(-lightSpaceOffset, lightSpaceOffset) };
				const float ty{ MathUtils::RandF(-lightSpaceOffset, lightSpaceOffset) };
				const float tz{ MathUtils::RandF(-lightSpaceOffset, lightSpaceOffset) };

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
				worldMatByGeomIndex[0][i] = world;
			}

			BuildConstantBuffers(*newTask, geometryBuffersCount);

			// Create geometry buffers SRV's
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
		}		
	}
	);
}

