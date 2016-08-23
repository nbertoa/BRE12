#include "BasicScene.h"

#include <algorithm>
#include <tbb/parallel_for.h>

#include <CommandManager/CommandManager.h>
#include <GeometryGenerator/GeometryGenerator.h>
#include <GlobalData/D3dData.h>
#include <PSOCreator/Material.h>
#include <PSOCreator/PSOCreator.h>
#include <PSOCreator/PunctualLight.h>
#include <ResourceManager/BufferCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <Scene/CmdListRecorders/BasicCmdListRecorder.h>
#include <Scene/CmdListRecorders/PunctualLightCmdListRecorder.h>

namespace {
	void BuildBuffers(BasicCmdListRecorder& task) noexcept {
		ASSERT(task.CbvSrvUavDescHeap() == nullptr);
#ifdef _DEBUG
		for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
			ASSERT(task.FrameCBuffer(i) == nullptr);
		}		
#endif
		ASSERT(task.ObjectCBuffer() == nullptr);
		ASSERT(task.MaterialsCBuffer() == nullptr);

		const std::uint32_t geomCount{ (std::uint32_t)task.GetVertexAndIndexBufferDataVec().size() };
		std::uint32_t numGeomDesc{ 0U };
		for (std::size_t i = 0UL; i < geomCount; ++i) {
			numGeomDesc += (std::uint32_t)task.WorldMatrices()[i].size();
		}
		ASSERT(numGeomDesc != 0U);

		// Create CBV_SRV_UAV cbuffer descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descHeapDesc.NodeMask = 0U;
		descHeapDesc.NumDescriptors = numGeomDesc * 2; // 1 obj cbuffer + 1 material cbuffer per geometry to draw
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, task.CbvSrvUavDescHeap());
		
		// Create object cbuffer and fill it
		const std::size_t objCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };
		ResourceManager::Get().CreateUploadBuffer(objCBufferElemSize, numGeomDesc, task.ObjectCBuffer());
		task.ObjectCBufferGpuDescHandleBegin() = task.CbvSrvUavDescHeap()->GetGPUDescriptorHandleForHeapStart();
		std::uint32_t k = 0U;
		for (std::size_t i = 0UL; i < geomCount; ++i) {
			const std::uint32_t worldMatsCount{ (std::uint32_t)task.WorldMatrices()[i].size() };
			for (std::uint32_t j = 0UL; j < worldMatsCount; ++j) {
				DirectX::XMFLOAT4X4 w;
				const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&task.WorldMatrices()[i][j]));
				DirectX::XMStoreFloat4x4(&w, wMatrix);
				task.ObjectCBuffer()->CopyData(k + j, &w, sizeof(w));
			}

			k += worldMatsCount;
		}

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

		// Create frame cbuffers
		const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4) * 2UL) };
		for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
			ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, task.FrameCBuffer(i));
		}
	}
}

void BasicScene::GenerateGeomPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept {
	ASSERT(tasks.empty());

	GeometryGenerator::MeshData shape;
	GeometryGenerator::CreateSphere(5.0f, 50U, 50U, shape);
	//GeometryGenerator::CreateCylinder(2.0f, 2.0f, 4, 20, 20, shape);
	//GeometryGenerator::CreateBox(2, 2, 2, 2, shape);
	//GeometryGenerator::CreateGrid(2, 2, 50, 50, shape);

	const std::size_t numGeometry{ 4000UL };
	tasks.resize(Settings::sCpuProcessors);

	// Create a command list 
	ID3D12GraphicsCommandList* cmdList;
	ID3D12CommandAllocator* cmdAlloc;
	CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc);
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc, cmdList);

	const PSOCreator::PSOData& psoData(PSOCreator::CommonPSOData::GetData(PSOCreator::CommonPSOData::BASIC));

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

	const float meshSpaceOffset{ 200.0f };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, Settings::sCpuProcessors, 1U),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			BasicCmdListRecorder& task{ *new BasicCmdListRecorder(D3dData::Device(), cmdListQueue) };
			tasks[k].reset(&task);
			task.PSO() = psoData.mPSO;
			task.RootSign() = psoData.mRootSign;
						
			task.GetVertexAndIndexBufferDataVec().resize(1UL, vAndIData);
			CmdListRecorder::MatricesVec& worldMatrices{ task.WorldMatrices() };
			worldMatrices.resize(1UL, CmdListRecorder::Matrices(numGeometry));
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				const float tx{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float ty{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float tz{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
				worldMatrices[0][i] = world;
			}

			BuildBuffers(task);

			ASSERT(task.ValidateData());
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

	const std::uint32_t numTasks{ 1U };
	tasks.resize(numTasks);

	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, numTasks, 1U),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			PunctualLightCmdListRecorder& task{ *new PunctualLightCmdListRecorder(D3dData::Device(), cmdListQueue) };
			tasks[k].reset(&task);
			PunctualLight light;
			light.mPosAndRange[0] = 0.0f;
			light.mPosAndRange[1] = 300.0f;
			light.mPosAndRange[2] = 0.0f;
			light.mPosAndRange[3] = 100000.0f;
			light.mColorAndPower[0] = 1.0f;
			light.mColorAndPower[1] = 1.0f;
			light.mColorAndPower[2] = 1.0f;
			light.mColorAndPower[3] = 1000000.0f;
			task.Init(geometryBuffers, geometryBuffersCount, &light, 1U);
		}		
	}
	);
}

