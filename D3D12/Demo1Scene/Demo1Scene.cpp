#include "Demo1Scene.h"

#include <algorithm>
#include <tbb/parallel_for.h>

#include <DXUtils/Material.h>
#include <DXUtils/MaterialFactory.h>
#include <DXUtils/PunctualLight.h>
#include <GlobalData/D3dData.h>
#include <MathUtils\MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Scene/CmdListRecorders/BasicCmdListRecorder.h>
#include <Scene/CmdListRecorders/HeightCmdListRecorder.h>
#include <Scene/CmdListRecorders/PunctualLightCmdListRecorder.h>

namespace {
	static const float sTx{ 0.0f };
	static const float sTy{ 0.0f };
	static const float sTz{ 50.0f };
	static const float sOffsetX{ 100.0f };

	void GenerateScene(		 
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		PunctualLightCmdListRecorder* &recorder) {
		recorder = new PunctualLightCmdListRecorder(D3dData::Device(), cmdListQueue);
		PunctualLight light[1];
		light[0].mPosAndRange[0] = 0.0f;
		light[0].mPosAndRange[1] = 300.0f;
		light[0].mPosAndRange[2] = -100.0f;
		light[0].mPosAndRange[3] = 100000.0f;
		light[0].mColorAndPower[0] = 1.0f;
		light[0].mColorAndPower[1] = 1.0f;
		light[0].mColorAndPower[2] = 1.0f;
		light[0].mColorAndPower[3] = 1000000.0f;

		recorder->Init(geometryBuffers, geometryBuffersCount, light, _countof(light));
	}

	void GenerateScene(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		Mesh& mesh1,
		Mesh& /*mesh2*/,
		BasicCmdListRecorder* &recorder) {
		recorder = new BasicCmdListRecorder(D3dData::Device(), cmdListQueue);
		
		CmdListRecorder::GeometryData geomData;
		geomData.mVertexBufferData = mesh1.VertexBufferData();
		geomData.mIndexBufferData = mesh1.IndexBufferData();
		geomData.mWorldMatrices.resize(MaterialFactory::NUM_MATERIALS);

		// Init world matrices and materials
		for (std::size_t i = 0UL; i < MaterialFactory::NUM_MATERIALS; ++i) {
			const float tx{ sTx };
			const float ty{ sTy };
			const float tz{ sTz * (i + 1) };
			MathUtils::ComputeMatrix(geomData.mWorldMatrices[i], tx, ty, tz);
		}

		recorder->Init(&geomData, 1U, &MaterialFactory::GetMaterial(MaterialFactory::GOLD), MaterialFactory::NUM_MATERIALS);
	}
}

void Demo1Scene::GenerateGeomPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& /*cmdListQueue*/,
	CmdListHelper& cmdListHelper,
	std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept {
	ASSERT(tasks.empty());

	const std::size_t numGeometry{ 100UL };
	tasks.resize(Settings::sCpuProcessors);

	// Load models
	Model* model1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer1;
	ModelManager::Get().CreateSphere(4.0f, 50, 50, model1, cmdListHelper.CmdList(), uploadVertexBuffer1, uploadIndexBuffer1);
	ASSERT(model1 != nullptr);
	ASSERT(model1->Meshes().size() == 1UL);
	//Mesh& mesh1{ *model1->Meshes()[0U] };

	Model* model2;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer2;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer2;
	ModelManager::Get().LoadModel("models/mitsubaFloor.obj", model2, cmdListHelper.CmdList(), uploadVertexBuffer2, uploadIndexBuffer2);
	ASSERT(model2 != nullptr);
	ASSERT(model2 != nullptr);
	ASSERT(model2->Meshes().size() == 1UL);
	//Mesh& mesh2{ *model2->Meshes()[0U] };

	const std::uint32_t numResources{ 7U };

	ID3D12Resource* tex[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/white.dds", tex[0], uploadBufferTex[0], cmdListHelper.CmdList());
	ASSERT(tex[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/white.dds", tex[1], uploadBufferTex[1], cmdListHelper.CmdList());
	ASSERT(tex[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/white.dds", tex[2], uploadBufferTex[2], cmdListHelper.CmdList());
	ASSERT(tex[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/white.dds", tex[3], uploadBufferTex[3], cmdListHelper.CmdList());
	ASSERT(tex[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/white.dds", tex[4], uploadBufferTex[4], cmdListHelper.CmdList());
	ASSERT(tex[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/white.dds", tex[5], uploadBufferTex[5], cmdListHelper.CmdList());
	ASSERT(tex[5] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/white.dds", tex[6], uploadBufferTex[6], cmdListHelper.CmdList());
	ASSERT(tex[6] != nullptr);

	ID3D12Resource* normal[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferNormal[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock_normal.dds", normal[0], uploadBufferNormal[0], cmdListHelper.CmdList());
	ASSERT(normal[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/bricks2_normal.dds", normal[1], uploadBufferNormal[1], cmdListHelper.CmdList());
	ASSERT(normal[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete_normal.dds", normal[2], uploadBufferNormal[2], cmdListHelper.CmdList());
	ASSERT(normal[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_normal.dds", normal[3], uploadBufferNormal[3], cmdListHelper.CmdList());
	ASSERT(normal[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/bricks_normal.dds", normal[4], uploadBufferNormal[4], cmdListHelper.CmdList());
	ASSERT(normal[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/bricks3_normal.dds", normal[5], uploadBufferNormal[5], cmdListHelper.CmdList());
	ASSERT(normal[5] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/stones_normal.dds", normal[6], uploadBufferNormal[6], cmdListHelper.CmdList());
	ASSERT(normal[6] != nullptr);

	ID3D12Resource* height[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferHeight[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock_height.dds", height[0], uploadBufferHeight[0], cmdListHelper.CmdList());
	ASSERT(height[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/bricks2_height.dds", height[1], uploadBufferHeight[1], cmdListHelper.CmdList());
	ASSERT(height[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete_height.dds", height[2], uploadBufferHeight[2], cmdListHelper.CmdList());
	ASSERT(height[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_height.dds", height[3], uploadBufferHeight[3], cmdListHelper.CmdList());
	ASSERT(height[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/bricks_height.dds", height[4], uploadBufferHeight[4], cmdListHelper.CmdList());
	ASSERT(height[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/bricks3_height.dds", height[5], uploadBufferHeight[5], cmdListHelper.CmdList());
	ASSERT(height[5] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/stones_height.dds", height[6], uploadBufferHeight[6], cmdListHelper.CmdList());
	ASSERT(height[6] != nullptr);

	cmdListHelper.ExecuteCmdList();

	/*ASSERT(model->HasMeshes());
	Mesh& mesh{ *model->Meshes()[0U] };

	std::vector<CmdListRecorder::GeometryData> geomDataVec;
	geomDataVec.resize(Settings::sCpuProcessors);
	for (CmdListRecorder::GeometryData& geomData : geomDataVec) {
		geomData.mVertexBufferData = mesh.VertexBufferData();
		geomData.mIndexBufferData = mesh.IndexBufferData();
		geomData.mWorldMatrices.reserve(numGeometry);
	}

	const float meshSpaceOffset{ 150.0f };
	const float scaleFactor{ 3.0f };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, Settings::sCpuProcessors, numGeometry),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			HeightCmdListRecorder& task{ *new HeightCmdListRecorder(D3dData::Device(), cmdListQueue) };
			tasks[k].reset(&task);

			CmdListRecorder::GeometryData& currGeomData{ geomDataVec[k] };
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				const float tx{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float ty{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };
				const float tz{ MathUtils::RandF(-meshSpaceOffset, meshSpaceOffset) };

				const float s{ scaleFactor };

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixScaling(s, s, s) * DirectX::XMMatrixTranslation(tx, ty, tz));
				currGeomData.mWorldMatrices.push_back(world);
			}

			std::vector<Material> materials;
			materials.reserve(numGeometry);
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				Material material;
				material.mBaseColor_MetalMask[0] = MathUtils::RandF(0.0f, 1.0f);
				material.mBaseColor_MetalMask[1] = MathUtils::RandF(0.0f, 1.0f);
				material.mBaseColor_MetalMask[2] = MathUtils::RandF(0.0f, 1.0f);
				material.mBaseColor_MetalMask[3] = (float)MathUtils::Rand(0U, 1U);
				const float f0Color{ MathUtils::RandF(0.0f, 0.4f) };
				material.mReflectance_Smoothness[0] = f0Color;
				material.mReflectance_Smoothness[1] = f0Color;
				material.mReflectance_Smoothness[2] = f0Color;
				material.mReflectance_Smoothness[3] = MathUtils::RandF(0.0f, 1.0f);
				materials.push_back(material);
			}

			std::vector<ID3D12Resource*> textures;
			textures.reserve(numGeometry);
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				textures.push_back(tex[i % numResources]);
			}

			std::vector<ID3D12Resource*> normals;
			normals.reserve(numGeometry);
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				normals.push_back(normal[i % numResources]);
			}

			std::vector<ID3D12Resource*> heights;
			heights.reserve(numGeometry);
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				heights.push_back(height[i % numResources]);
			}

			task.Init(&currGeomData, 1U, materials.data(), textures.data(), normals.data(), heights.data(), (std::uint32_t)normals.size());
		}
	}
	);*/
}

void Demo1Scene::GenerateLightPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	tasks.resize(1UL);
	PunctualLightCmdListRecorder* recorder{ nullptr };
	GenerateScene(cmdListQueue, geometryBuffers, geometryBuffersCount, recorder);
	ASSERT(recorder != nullptr);
	tasks[0].reset(recorder);
}

