#include "BasicScene.h"

#include <algorithm>
#include <tbb/parallel_for.h>

#include <CommandManager/CommandManager.h>
#include <GeometryGenerator/GeometryGenerator.h>
#include <GlobalData/D3dData.h>
#include <PSOCreator/Material.h>
#include <PSOCreator/PunctualLight.h>
#include <ResourceManager/BufferCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <Scene/CmdListRecorders/BasicCmdListRecorder.h>
#include <Scene/CmdListRecorders/PunctualLightCmdListRecorder.h>

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

	std::vector<CmdListRecorder::GeometryData> geomDataVec;
	geomDataVec.resize(Settings::sCpuProcessors);
	for (CmdListRecorder::GeometryData& geomData : geomDataVec) {
		geomData.mVertexBufferData = vertexBufferData;
		geomData.mIndexBufferData = indexBufferData;
		geomData.mWorldMatrices.reserve(numGeometry);
	}

	const float meshSpaceOffset{ 200.0f };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, Settings::sCpuProcessors, 1U),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			BasicCmdListRecorder& task{ *new BasicCmdListRecorder(D3dData::Device(), cmdListQueue) };
			tasks[k].reset(&task);
						
			CmdListRecorder::GeometryData& currGeomData{ geomDataVec[k] };
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				const float tx{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float ty{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float tz{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
				currGeomData.mWorldMatrices.push_back(world);
			}

			std::vector<Material> materials;
			materials.reserve(numGeometry);
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				Material material;
				material.mBaseColor_MetalMask[0] = MathUtils::RandF(0.0f, 1.0f);
				material.mBaseColor_MetalMask[1] = MathUtils::RandF(0.0f, 1.0f);
				material.mBaseColor_MetalMask[2] = MathUtils::RandF(0.0f, 1.0f);
				material.mBaseColor_MetalMask[3] = MathUtils::RandF(0.0f, 1.0f);
				material.mReflectance_Smoothness[0] = MathUtils::RandF(0.0f, 1.0f);
				material.mReflectance_Smoothness[1] = MathUtils::RandF(0.0f, 1.0f);
				material.mReflectance_Smoothness[2] = MathUtils::RandF(0.0f, 1.0f);
				material.mReflectance_Smoothness[3] = MathUtils::RandF(0.0f, 1.0f);
				materials.push_back(material);
			}

			task.Init(&currGeomData, 1U, materials.data(), (std::uint32_t)materials.size());
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

