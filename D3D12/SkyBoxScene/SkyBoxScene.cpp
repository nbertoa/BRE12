#include "SkyBoxScene.h"

#include <tbb/parallel_for.h>

#include <GeometryPass/Material.h>
#include <GeometryPass/Recorders/HeightCmdListRecorder.h>
#include <GlobalData/D3dData.h>
#include <LightPass/PunctualLight.h>
#include <LightPass/Recorders/PunctualLightCmdListRecorder.h>
#include <MathUtils\MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <SkyBoxPass/SkyBoxCmdListRecorder.h>

namespace {
	const char* sSkyBoxFile{ "textures/milkmill_cube_map.dds" };
	const char* sDiffuseEnvironmentFile{ "textures/milkmill_diffuse_cube_map.dds" };
	const char* sSpecularEnvironmentFile{ "textures/milkmill_specular_cube_map.dds" };

	const float sS{ 2.0f };

	const float sTx{ 0.0f };
	const float sTy{ -3.5f };
	const float sTz{ 10.0f };	
	const float sOffsetX{ 25.0f };

	const float sTx1{ 0.0f };
	const float sTy1{ -3.5f };
	const float sTz1{ 30.0f };
	const float sOffsetX1{ 25.0f };

	const float sTx2{ 0.0f };
	const float sTy2{ -3.5f };
	const float sTz2{ 15.0f };
	const float sOffsetX2{ 25.0f };

	const float sTx3{ 0.0f };
	const float sTy3{ -3.5f };
	const float sTz3{ 0.0f };
	const float sOffsetX3{ 15.0f };

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
		ID3D12Resource& diffuseCubeMap,
		ID3D12Resource& specularCubeMap,
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

		recorder->Init(
			geomDataVec.data(), 
			static_cast<std::uint32_t>(geomDataVec.size()),
			materialsVec.data(), 
			texturesVec.data(), 
			normalsVec.data(), 
			heightsVec.data(), 
			static_cast<std::uint32_t>(materialsVec.size()),
			diffuseCubeMap,
			specularCubeMap);
	}
}

void SkyBoxScene::GenerateGeomPassRecorders(
	ID3D12CommandQueue& cmdQueue,
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {

	ASSERT(tasks.empty());
	ASSERT(ValidateData());

	CHECK_HR(mCmdList->Reset(mCmdAlloc, nullptr));
	
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().CreateSphere(2, 50, 50, model, *mCmdList, uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);

	Model* model1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer1;
	ModelManager::Get().LoadModel("models/mitsubaFloor.obj", model1, *mCmdList, uploadVertexBuffer1, uploadIndexBuffer1);
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
	ResourceManager::Get().LoadTextureFromFile("textures/rock.dds", tex[0], uploadBufferTex[0], *mCmdList);
	ASSERT(tex[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2.dds", tex[1], uploadBufferTex[1], *mCmdList);
	ASSERT(tex[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/wood.dds", tex[2], uploadBufferTex[2], *mCmdList);
	ASSERT(tex[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor.dds", tex[3], uploadBufferTex[3], *mCmdList);
	ASSERT(tex[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/sand.dds", tex[4], uploadBufferTex[4], *mCmdList);
	ASSERT(tex[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete.dds", tex[5], uploadBufferTex[5], *mCmdList);
	ASSERT(tex[5] != nullptr);

	ID3D12Resource* normal[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferNormal[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock_normal.dds", normal[0], uploadBufferNormal[0], *mCmdList);
	ASSERT(normal[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2_normal.dds", normal[1], uploadBufferNormal[1], *mCmdList);
	ASSERT(normal[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/wood_normal.dds", normal[2], uploadBufferNormal[2], *mCmdList);
	ASSERT(normal[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_normal.dds", normal[3], uploadBufferNormal[3], *mCmdList);
	ASSERT(normal[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/sand_normal.dds", normal[4], uploadBufferNormal[4], *mCmdList);
	ASSERT(normal[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete_normal.dds", normal[5], uploadBufferNormal[5], *mCmdList);
	ASSERT(normal[5] != nullptr);

	ID3D12Resource* height[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferHeight[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock_height.dds", height[0], uploadBufferHeight[0], *mCmdList);
	ASSERT(height[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2_height.dds", height[1], uploadBufferHeight[1], *mCmdList);
	ASSERT(height[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/wood_height.dds", height[2], uploadBufferHeight[2], *mCmdList);
	ASSERT(height[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_height.dds", height[3], uploadBufferHeight[3], *mCmdList);
	ASSERT(height[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/sand_height.dds", height[4], uploadBufferHeight[4], *mCmdList);
	ASSERT(height[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete_height.dds", height[5], uploadBufferHeight[5], *mCmdList);
	ASSERT(height[5] != nullptr);

	// Cube map textures
	ID3D12Resource* diffuseCubeMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex1;
	ResourceManager::Get().LoadTextureFromFile(sDiffuseEnvironmentFile, diffuseCubeMap, uploadBufferTex1, *mCmdList);
	ASSERT(diffuseCubeMap != nullptr);

	ID3D12Resource* specularCubeMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex2;
	ResourceManager::Get().LoadTextureFromFile(sSpecularEnvironmentFile, specularCubeMap, uploadBufferTex2, *mCmdList);
	ASSERT(specularCubeMap != nullptr);

	ExecuteCommandList(cmdQueue);

	tasks.resize(2);

	HeightCmdListRecorder* heightRecorder{ nullptr };
	GenerateRecorder(sTx1, sTy1, sTz1, sOffsetX1, 0.0f, 0.0f, cmdListQueue, model1->Meshes(), tex, normal, height, materials, numResources, *diffuseCubeMap, *specularCubeMap, heightRecorder);
	ASSERT(heightRecorder != nullptr);
	tasks[0].reset(heightRecorder);

	HeightCmdListRecorder* heightRecorder2{ nullptr };
	GenerateRecorder(sTx2, sTy2, sTz2, sOffsetX2, 0.0f, 0.0f, cmdListQueue, model->Meshes(), tex, normal, height, materials, numResources, *diffuseCubeMap, *specularCubeMap, heightRecorder2);
	ASSERT(heightRecorder2 != nullptr);
	tasks[1].reset(heightRecorder2);
}

void SkyBoxScene::GenerateLightPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	std::vector<std::unique_ptr<LightPassCmdListRecorder>>& tasks) noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	ASSERT(ValidateData());
	
	tasks.resize(1UL);
	PunctualLightCmdListRecorder* recorder{ nullptr };
	GenerateRecorder(cmdListQueue, geometryBuffers, geometryBuffersCount, recorder);
	ASSERT(recorder != nullptr);
	tasks[0].reset(recorder);
}

void SkyBoxScene::GenerateSkyBoxRecorder(
	ID3D12CommandQueue& cmdQueue,
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	std::unique_ptr<SkyBoxCmdListRecorder>& task) noexcept {

	ASSERT(ValidateData());

	CHECK_HR(mCmdList->Reset(mCmdAlloc, nullptr));

	SkyBoxCmdListRecorder* recorder = new SkyBoxCmdListRecorder(D3dData::Device(), cmdListQueue);

	// Sky box sphere
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().CreateSphere(3000, 50, 50, model, *mCmdList, uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);
	const std::vector<Mesh>& meshes(model->Meshes());
	ASSERT(meshes.size() == 1UL);

	// Cube map texture
	ID3D12Resource* cubeMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex;
	ResourceManager::Get().LoadTextureFromFile(sSkyBoxFile, cubeMap, uploadBufferTex, *mCmdList);
	ASSERT(cubeMap != nullptr);
	
	ExecuteCommandList(cmdQueue);

	// Build world matrix
	const Mesh& mesh{ meshes[0] };
	DirectX::XMFLOAT4X4 w;
	MathUtils::ComputeMatrix(w, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// Init recorder and store in task
	recorder->Init(mesh.VertexBufferData(), mesh.IndexBufferData(), w, *cubeMap);
	task.reset(recorder);
}

void SkyBoxScene::GenerateDiffuseAndSpecularCubeMaps(
	ID3D12CommandQueue& cmdQueue,
	ID3D12Resource* &diffuseIrradianceCubeMap,
	ID3D12Resource* &specularPreConvolvedCubeMap) noexcept
{
	CHECK_HR(mCmdList->Reset(mCmdAlloc, nullptr));

	// Cube map textures
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex;
	ResourceManager::Get().LoadTextureFromFile(sDiffuseEnvironmentFile, diffuseIrradianceCubeMap, uploadBufferTex, *mCmdList);
	ASSERT(diffuseIrradianceCubeMap != nullptr);

	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex2;
	ResourceManager::Get().LoadTextureFromFile(sSpecularEnvironmentFile, specularPreConvolvedCubeMap, uploadBufferTex2, *mCmdList);
	ASSERT(specularPreConvolvedCubeMap != nullptr);

	ExecuteCommandList(cmdQueue);
}

