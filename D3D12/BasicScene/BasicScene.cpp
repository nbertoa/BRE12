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
	void CreateGeometryBuffersSRVs(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers, 
		const std::uint32_t geometryBuffersCount, 
		D3D12_CPU_DESCRIPTOR_HANDLE baseCpuDescHandle) {
		ASSERT(geometryBuffers != nullptr);
		ASSERT(baseCpuDescHandle.ptr != 0UL);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		for (std::uint32_t i = 0U; i < geometryBuffersCount; ++i) {
			ID3D12Resource& res{ *geometryBuffers[i].Get() };

			srvDesc.Format = res.GetDesc().Format;
			srvDesc.Texture2D.MipLevels = res.GetDesc().MipLevels;

			ResourceManager::Get().CreateShaderResourceView(res, srvDesc, baseCpuDescHandle);

			baseCpuDescHandle.ptr += descHandleIncSize;
		}
	}

	void CreateLightsBufferSRV(ID3D12Resource& res, const std::uint32_t numLights, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandle) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = res.GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0UL;
		srvDesc.Buffer.NumElements = numLights;
		srvDesc.Buffer.StructureByteStride = sizeof(PunctualLight);
		ResourceManager::Get().CreateShaderResourceView(res, srvDesc, cpuDescHandle);
	}

	void BuildBuffers(BasicCmdListRecorder& task) noexcept {
		ASSERT(task.CbvSrvUavDescHeap() == nullptr);
		ASSERT(task.FrameCBuffer() == nullptr);
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

		// Create frame cbuffer
		const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4) * 2UL) };
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, task.FrameCBuffer());
	}

	void BuildBuffers(PunctualLightCmdListRecorder& task, const std::uint32_t descHeapOffset) noexcept {
		ASSERT(task.CbvSrvUavDescHeap() == nullptr);
		ASSERT(task.FrameCBuffer() == nullptr);
		ASSERT(task.LightsBuffer() == nullptr);

		// We assume we have world matrices by geometry index 0 (but we do not have geometry here)
		ASSERT(task.NumLights() != 0U);
		const std::uint32_t lightsCount{ task.NumLights() };

		// Create CBV_SRV_UAV cbuffer descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descHeapDesc.NodeMask = 0U;
		descHeapDesc.NumDescriptors = descHeapOffset + lightsCount;
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, task.CbvSrvUavDescHeap());

		// Create lights buffer and fill it
		const std::size_t lightBufferElemSize{ sizeof(PunctualLight) };
		ResourceManager::Get().CreateUploadBuffer(lightBufferElemSize, lightsCount, task.LightsBuffer());
		const float posOffset{ 50.0f };
		const float rangeOffset{ 10.0f };
		const float powerOffset{ 1000.0f };
		PunctualLight light;
		for (std::uint32_t i = 0UL; i < lightsCount; ++i) {
			light.mPosAndRange[0] = MathUtils::RandF(-posOffset, posOffset);
			light.mPosAndRange[1] = MathUtils::RandF(-posOffset, posOffset);
			light.mPosAndRange[2] = MathUtils::RandF(-posOffset, posOffset);
			light.mPosAndRange[3] = MathUtils::RandF(rangeOffset, rangeOffset * 2U);
			light.mColorAndPower[0] = MathUtils::RandF(0.5f, 1.0f);
			light.mColorAndPower[1] = MathUtils::RandF(0.5f, 1.0f);
			light.mColorAndPower[2] = MathUtils::RandF(0.5f, 1.0f);
			light.mColorAndPower[3] = MathUtils::RandF(powerOffset, powerOffset * 2U);
			task.LightsBuffer()->CopyData(i, &light, sizeof(PunctualLight));
		}

		// Create frame cbuffer
		const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4) * 2UL) };
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, task.FrameCBuffer());
	}
}

void BasicScene::GenerateGeomPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept {
	ASSERT(tasks.empty());

	GeometryGenerator::MeshData shape;
	GeometryGenerator::CreateSphere(2.0f, 50U, 50U, shape);
	//GeometryGenerator::CreateCylinder(2.0f, 2.0f, 4, 20, 20, shape);
	//GeometryGenerator::CreateBox(2, 2, 2, 2, shape);

	const std::size_t numGeometry{ 1000UL };
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

	const float meshSpaceOffset{ 50.0f };
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

	const PSOCreator::PSOData& psoData(PSOCreator::CommonPSOData::GetData(PSOCreator::CommonPSOData::PUNCTUAL_LIGHT));

	const std::uint32_t numLights{ 25 };
	tasks.resize(Settings::sCpuProcessors);

	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, Settings::sCpuProcessors, 1U),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			PunctualLightCmdListRecorder& task{ *new PunctualLightCmdListRecorder(D3dData::Device(), cmdListQueue) };
			tasks[k].reset(&task);
			task.PSO() = psoData.mPSO;
			task.RootSign() = psoData.mRootSign;
			task.NumLights() = numLights;

			const std::uint32_t descHeapOffset(geometryBuffersCount + 1U);
			BuildBuffers(task, descHeapOffset);

			// Create geometry buffers SRVs
			D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvUavCpuDescHandle(task.CbvSrvUavDescHeap()->GetCPUDescriptorHandleForHeapStart());
			CreateGeometryBuffersSRVs(geometryBuffers, geometryBuffersCount, cbvSrvUavCpuDescHandle);
			
			// Create lights buffer SRV
			const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
			D3D12_CPU_DESCRIPTOR_HANDLE lightsBufferCpuDescHandle{ cbvSrvUavCpuDescHandle.ptr + geometryBuffersCount * descHandleIncSize };
			CreateLightsBufferSRV(*task.LightsBuffer()->Resource(), task.NumLights(), lightsBufferCpuDescHandle);
			task.LightsBufferGpuDescHandleBegin().ptr = task.CbvSrvUavDescHeap()->GetGPUDescriptorHandleForHeapStart().ptr + geometryBuffersCount * descHandleIncSize;

			ASSERT(task.ValidateData());
		}		
	}
	);
}

