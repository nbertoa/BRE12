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
#include <Scene/GeometryPass/HeightCmdListRecorder.h>
#include <Scene/LightPass/PunctualLightCmdListRecorder.h>

namespace {
	static const float sS{ 2.0f };

	static const float sTx{ 0.0f };
	static const float sTy{ -3.5f };
	static const float sTz{ 10.0f };	
	static const float sOffsetX{ 25.0f };

	static const float sTx1{ 0.0f };
	static const float sTy1{ -3.5f };
	static const float sTz1{ 30.0f };
	static const float sOffsetX1{ 25.0f };

	static const float sTx2{ 0.0f };
	static const float sTy2{ -3.5f };
	static const float sTz2{ 15.0f };
	static const float sOffsetX2{ 25.0f };

	static const float sTx3{ 0.0f };
	static const float sTy3{ -3.5f };
	static const float sTz3{ 0.0f };
	static const float sOffsetX3{ 15.0f };

	void GenerateRecorder(		 
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		PunctualLightCmdListRecorder* &recorder) {
		recorder = new PunctualLightCmdListRecorder(D3dData::Device(), cmdListQueue);
		PunctualLight light[1];
		light[0].mPosAndRange[0] = 0.0f;
		light[0].mPosAndRange[1] = 300.0f;
		light[0].mPosAndRange[2] = -100.0f;
		light[0].mPosAndRange[3] = 10000.0f;
		light[0].mColorAndPower[0] = 1.0f;
		light[0].mColorAndPower[1] = 1.0f;
		light[0].mColorAndPower[2] = 1.0f;
		light[0].mColorAndPower[3] = 1000000.0f;

		recorder->Init(geometryBuffers, geometryBuffersCount, light, _countof(light));
	}

	void GenerateRecorder(
		const float initX,
		const float initY,
		const float initZ,
		const float offsetX,
		const float offsetY,
		const float offsetZ,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		const std::vector<Mesh>& meshes,
		ID3D12Resource** textures,
		ID3D12Resource** normals,
		ID3D12Resource** heights,
		Material* materials,
		const std::size_t numMaterials,
		HeightCmdListRecorder* &recorder) {

		ASSERT(textures != nullptr);
		ASSERT(normals != nullptr);
		ASSERT(heights != nullptr);

		recorder = new HeightCmdListRecorder(D3dData::Device(), cmdListQueue);
		
		const std::size_t numMeshes{ meshes.size() };
		ASSERT(numMeshes > 0UL);

		std::vector<GeometryPassCmdListRecorder::GeometryData> geomDataVec;
		geomDataVec.resize(numMeshes);
		for (std::size_t i = 0UL; i < numMeshes; ++i) {
			GeometryPassCmdListRecorder::GeometryData& geomData{ geomDataVec[i] };
			const Mesh& mesh{ meshes[i] };
			geomData.mVertexBufferData = mesh.VertexBufferData();
			geomData.mIndexBufferData = mesh.IndexBufferData();
			geomData.mWorldMatrices.reserve(numMaterials);
		}

		std::vector<Material> materialsVec;
		materialsVec.resize(numMaterials * numMeshes);
		std::vector<ID3D12Resource*> texturesVec;
		texturesVec.resize(numMaterials * numMeshes);
		std::vector<ID3D12Resource*> normalsVec;
		normalsVec.resize(numMaterials * numMeshes);
		std::vector<ID3D12Resource*> heightsVec;
		heightsVec.resize(numMaterials * numMeshes);

		float tx{ initX };
		float ty{ initY };
		float tz{ initZ };
		for (std::size_t i = 0UL; i < numMaterials; ++i) {
			DirectX::XMFLOAT4X4 w;
			MathUtils::ComputeMatrix(w, tx, ty, tz, sS, sS, sS);

			Material& mat(materials[i]);
			ID3D12Resource* texture{ textures[i] };
			ID3D12Resource* normal{ normals[i] };
			ID3D12Resource* height{ heights[i] };
			for (std::size_t j = 0UL; j < numMeshes; ++j) {
				const std::size_t index{ i + j * numMaterials };
				materialsVec[index] = mat;
				texturesVec[index] = texture;
				normalsVec[index] = normal;
				heightsVec[index] = height;
				GeometryPassCmdListRecorder::GeometryData& geomData{ geomDataVec[j] };
				geomData.mWorldMatrices.push_back(w);
			}

			tx += offsetX;
			ty += offsetY;
			tz += offsetZ;
		}

		recorder->Init(geomDataVec.data(), (std::uint32_t)geomDataVec.size(), materialsVec.data(), texturesVec.data(), normalsVec.data(), heightsVec.data(), (std::uint32_t)materialsVec.size());
	}
}

void Demo1Scene::GenerateGeomPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	CmdListHelper& cmdListHelper,
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) const noexcept {
	ASSERT(tasks.empty());
	
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().CreateSphere(2, 50, 50, model, cmdListHelper.CmdList(), uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);

	Model* model1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer1;
	ModelManager::Get().LoadModel("models/mitsubaFloor.obj", model1, cmdListHelper.CmdList(), uploadVertexBuffer1, uploadIndexBuffer1);
	ASSERT(model1 != nullptr);

	const std::uint32_t numResources{ 6U };

	Material materials[numResources];
	for (std::uint32_t i = 0UL; i < numResources; ++i) {
		materials[i].mBaseColor_MetalMask[3U] = 0.0f;
		materials[i].mSmoothness = 0.7f;
	}

	materials[4].mSmoothness = 0.2f;
	materials[5].mSmoothness = 0.2f;

	ID3D12Resource* tex[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock.dds", tex[0], uploadBufferTex[0], cmdListHelper.CmdList());
	ASSERT(tex[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2.dds", tex[1], uploadBufferTex[1], cmdListHelper.CmdList());
	ASSERT(tex[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/wood.dds", tex[2], uploadBufferTex[2], cmdListHelper.CmdList());
	ASSERT(tex[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor.dds", tex[3], uploadBufferTex[3], cmdListHelper.CmdList());
	ASSERT(tex[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/sand.dds", tex[4], uploadBufferTex[4], cmdListHelper.CmdList());
	ASSERT(tex[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete.dds", tex[5], uploadBufferTex[5], cmdListHelper.CmdList());
	ASSERT(tex[5] != nullptr);

	ID3D12Resource* normal[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferNormal[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock_normal.dds", normal[0], uploadBufferNormal[0], cmdListHelper.CmdList());
	ASSERT(normal[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2_normal.dds", normal[1], uploadBufferNormal[1], cmdListHelper.CmdList());
	ASSERT(normal[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/wood_normal.dds", normal[2], uploadBufferNormal[2], cmdListHelper.CmdList());
	ASSERT(normal[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_normal.dds", normal[3], uploadBufferNormal[3], cmdListHelper.CmdList());
	ASSERT(normal[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/sand_normal.dds", normal[4], uploadBufferNormal[4], cmdListHelper.CmdList());
	ASSERT(normal[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete_normal.dds", normal[5], uploadBufferNormal[5], cmdListHelper.CmdList());
	ASSERT(normal[5] != nullptr);

	ID3D12Resource* height[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferHeight[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock_height.dds", height[0], uploadBufferHeight[0], cmdListHelper.CmdList());
	ASSERT(height[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2_height.dds", height[1], uploadBufferHeight[1], cmdListHelper.CmdList());
	ASSERT(height[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/wood_height.dds", height[2], uploadBufferHeight[2], cmdListHelper.CmdList());
	ASSERT(height[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_height.dds", height[3], uploadBufferHeight[3], cmdListHelper.CmdList());
	ASSERT(height[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/sand_height.dds", height[4], uploadBufferHeight[4], cmdListHelper.CmdList());
	ASSERT(height[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete_height.dds", height[5], uploadBufferHeight[5], cmdListHelper.CmdList());
	ASSERT(height[5] != nullptr);

	cmdListHelper.ExecuteCmdList();

	tasks.resize(2);

	HeightCmdListRecorder* heightRecorder{ nullptr };
	GenerateRecorder(sTx1, sTy1, sTz1, sOffsetX1, 0.0f, 0.0f, cmdListQueue, model1->Meshes(), tex, normal, height, materials, numResources, heightRecorder);
	ASSERT(heightRecorder != nullptr);
	tasks[0].reset(heightRecorder);

	HeightCmdListRecorder* heightRecorder2{ nullptr };
	GenerateRecorder(sTx2, sTy2, sTz2, sOffsetX2, 0.0f, 0.0f, cmdListQueue, model->Meshes(), tex, normal, height, materials, numResources, heightRecorder2);
	ASSERT(heightRecorder2 != nullptr);
	tasks[1].reset(heightRecorder2);
}

void Demo1Scene::GenerateLightPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	CmdListHelper& /*cmdListHelper*/,
	std::vector<std::unique_ptr<LightPassCmdListRecorder>>& tasks) const noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	
	tasks.resize(1UL);
	PunctualLightCmdListRecorder* recorder{ nullptr };
	GenerateRecorder(cmdListQueue, geometryBuffers, geometryBuffersCount, recorder);
	ASSERT(recorder != nullptr);
	tasks[0].reset(recorder);
}

