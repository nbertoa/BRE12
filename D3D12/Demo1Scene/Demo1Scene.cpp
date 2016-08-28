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
#include <Scene/CmdListRecorders/NormalCmdListRecorder.h>
#include <Scene/CmdListRecorders/PunctualLightCmdListRecorder.h>

namespace {
	static const float sTx{ 0.0f };
	static const float sTy{ -3.5f };
	static const float sTz{ 10.0f };
	static const float sS{ 2.0f };
	static const float sOffsetX{ 25.0f };
	static const std::size_t basicGeomOffset{ 0UL };
	static const std::size_t normalGeomOffset{ 7UL };
	static const std::size_t heightGeomOffset{ 14UL };

	void GenerateRecorder(		 
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		PunctualLightCmdListRecorder* &recorder) {
		recorder = new PunctualLightCmdListRecorder(D3dData::Device(), cmdListQueue);
		PunctualLight light[2];
		light[0].mPosAndRange[0] = 0.0f;
		light[0].mPosAndRange[1] = 300.0f;
		light[0].mPosAndRange[2] = -100.0f;
		light[0].mPosAndRange[3] = 100000.0f;
		light[0].mColorAndPower[0] = 1.0f;
		light[0].mColorAndPower[1] = 1.0f;
		light[0].mColorAndPower[2] = 1.0f;
		light[0].mColorAndPower[3] = 1000000.0f;

		light[1].mPosAndRange[0] = 200.0f;
		light[1].mPosAndRange[1] = 300.0f;
		light[1].mPosAndRange[2] = -100.0f;
		light[1].mPosAndRange[3] = 100000.0f;
		light[1].mColorAndPower[0] = 1.0f;
		light[1].mColorAndPower[1] = 1.0f;
		light[1].mColorAndPower[2] = 1.0f;
		light[1].mColorAndPower[3] = 1000000.0f;

		recorder->Init(geometryBuffers, geometryBuffersCount, light, _countof(light));
	}

	void GenerateRecorder(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		const std::vector<Mesh>& meshes,
		BasicCmdListRecorder* &recorder) {
		recorder = new BasicCmdListRecorder(D3dData::Device(), cmdListQueue);

		const std::size_t numMaterials(MaterialFactory::NUM_MATERIALS - 1);
		
		const std::size_t numMeshes{ meshes.size() };
		ASSERT(numMeshes > 0UL);

		std::vector<CmdListRecorder::GeometryData> geomDataVec;
		geomDataVec.resize(numMeshes);		
		for (std::size_t i = 0UL; i < numMeshes; ++i) {
			CmdListRecorder::GeometryData& geomData{ geomDataVec[i] };
			const Mesh& mesh{ meshes[i] };
			geomData.mVertexBufferData = mesh.VertexBufferData();
			geomData.mIndexBufferData = mesh.IndexBufferData();
			geomData.mWorldMatrices.reserve(numMaterials);
		}

		std::vector<Material> materials;
		materials.resize(numMaterials * numMeshes);
		for (std::size_t i = 0UL; i < numMaterials; ++i) {
			const float tx{ sTx + (i + basicGeomOffset) * sOffsetX };
			const float ty{ sTy };
			const float tz{ sTz };
			DirectX::XMFLOAT4X4 w;
			MathUtils::ComputeMatrix(w, tx, ty, tz, sS, sS, sS);

			Material mat(MaterialFactory::GetMaterial((MaterialFactory::MaterialType)i));
			for (std::size_t j = 0UL; j < numMeshes; ++j) {				
				materials[i + j * numMaterials] = mat;
				CmdListRecorder::GeometryData& geomData{ geomDataVec[j] };
				geomData.mWorldMatrices.push_back(w);
			}
		}

		recorder->Init(geomDataVec.data(), (std::uint32_t)geomDataVec.size(), materials.data(), (std::uint32_t)materials.size());
	}

	void GenerateRecorder(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		const std::vector<Mesh>& meshes,
		ID3D12Resource** textures,
		ID3D12Resource** normals,
		NormalCmdListRecorder* &recorder) {

		ASSERT(textures != nullptr);
		ASSERT(normals != nullptr);

		recorder = new NormalCmdListRecorder(D3dData::Device(), cmdListQueue);

		const std::size_t numMaterials(MaterialFactory::NUM_MATERIALS - 1);

		const std::size_t numMeshes{ meshes.size() };
		ASSERT(numMeshes > 0UL);

		std::vector<CmdListRecorder::GeometryData> geomDataVec;
		geomDataVec.resize(numMeshes);
		for (std::size_t i = 0UL; i < numMeshes; ++i) {
			CmdListRecorder::GeometryData& geomData{ geomDataVec[i] };
			const Mesh& mesh{ meshes[i] };
			geomData.mVertexBufferData = mesh.VertexBufferData();
			geomData.mIndexBufferData = mesh.IndexBufferData();
			geomData.mWorldMatrices.reserve(numMaterials);
		}

		std::vector<Material> materials;
		materials.resize(numMaterials * numMeshes);
		std::vector<ID3D12Resource*> texturesVec;
		texturesVec.resize(numMaterials * numMeshes);
		std::vector<ID3D12Resource*> normalsVec;
		normalsVec.resize(numMaterials * numMeshes);
		for (std::size_t i = 0UL; i < numMaterials; ++i) {
			const float tx{ sTx + (i + normalGeomOffset) * sOffsetX };
			const float ty{ sTy };
			const float tz{ sTz };
			DirectX::XMFLOAT4X4 w;
			MathUtils::ComputeMatrix(w, tx, ty, tz, sS, sS, sS);

			Material mat(MaterialFactory::GetMaterial((MaterialFactory::MaterialType)i));
			ID3D12Resource* texture{ textures[i] };
			ID3D12Resource* normal{ normals[i] };
			for (std::size_t j = 0UL; j < numMeshes; ++j) {	
				const std::size_t index{ i + j * numMaterials };
				materials[index] = mat;
				texturesVec[index] = texture;
				normalsVec[index] = normal;
				CmdListRecorder::GeometryData& geomData{ geomDataVec[j] };
				geomData.mWorldMatrices.push_back(w);
			}
		}

		recorder->Init(geomDataVec.data(), (std::uint32_t)geomDataVec.size(), materials.data(), texturesVec.data(), normalsVec.data(), (std::uint32_t)materials.size());
	}

	void GenerateRecorder(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		const std::vector<Mesh>& meshes,
		ID3D12Resource** textures,
		ID3D12Resource** normals,
		ID3D12Resource** heights,
		HeightCmdListRecorder* &recorder) {

		ASSERT(textures != nullptr);
		ASSERT(normals != nullptr);
		ASSERT(heights != nullptr);

		recorder = new HeightCmdListRecorder(D3dData::Device(), cmdListQueue);

		const std::size_t numMaterials(MaterialFactory::NUM_MATERIALS - 1);

		const std::size_t numMeshes{ meshes.size() };
		ASSERT(numMeshes > 0UL);

		std::vector<CmdListRecorder::GeometryData> geomDataVec;
		geomDataVec.resize(numMeshes);
		for (std::size_t i = 0UL; i < numMeshes; ++i) {
			CmdListRecorder::GeometryData& geomData{ geomDataVec[i] };
			const Mesh& mesh{ meshes[i] };
			geomData.mVertexBufferData = mesh.VertexBufferData();
			geomData.mIndexBufferData = mesh.IndexBufferData();
			geomData.mWorldMatrices.reserve(numMaterials);
		}

		std::vector<Material> materials;
		materials.resize(numMaterials * numMeshes);
		std::vector<ID3D12Resource*> texturesVec;
		texturesVec.resize(numMaterials * numMeshes);
		std::vector<ID3D12Resource*> normalsVec;
		normalsVec.resize(numMaterials * numMeshes);
		std::vector<ID3D12Resource*> heightsVec;
		heightsVec.resize(numMaterials * numMeshes);
		for (std::size_t i = 0UL; i < numMaterials; ++i) {
			const float tx{ sTx + (i + heightGeomOffset) * sOffsetX };
			const float ty{ sTy };
			const float tz{ sTz };
			DirectX::XMFLOAT4X4 w;
			MathUtils::ComputeMatrix(w, tx, ty, tz, sS, sS, sS);

			Material mat(MaterialFactory::GetMaterial((MaterialFactory::MaterialType)i));
			ID3D12Resource* texture{ textures[i] };
			ID3D12Resource* normal{ normals[i] };
			ID3D12Resource* height{ heights[i] };
			for (std::size_t j = 0UL; j < numMeshes; ++j) {
				const std::size_t index{ i + j * numMaterials };
				materials[index] = mat;
				texturesVec[index] = texture;
				normalsVec[index] = normal;
				heightsVec[index] = height;
				CmdListRecorder::GeometryData& geomData{ geomDataVec[j] };
				geomData.mWorldMatrices.push_back(w);
			}
		}

		recorder->Init(geomDataVec.data(), (std::uint32_t)geomDataVec.size(), materials.data(), texturesVec.data(), normalsVec.data(), heightsVec.data(), (std::uint32_t)materials.size());
	}
}

void Demo1Scene::GenerateGeomPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	CmdListHelper& cmdListHelper,
	std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept {
	ASSERT(tasks.empty());

	/*Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().LoadModel("models/mitsubaFloor.obj", model, cmdListHelper.CmdList(), uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);*/

	std::vector<Mesh> meshes;

	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().CreateSphere(4, 50, 50, model, cmdListHelper.CmdList(), uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);
	meshes.push_back(model->Meshes()[0U]);


	Model* model1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer1;
	ModelManager::Get().LoadModel("models/mitsubaFloor.obj", model1, cmdListHelper.CmdList(), uploadVertexBuffer1, uploadIndexBuffer1);
	ASSERT(model1 != nullptr);
	meshes.push_back(model1->Meshes()[0U]);

	const std::uint32_t numResources{ 7U };

	ID3D12Resource* tex[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex[numResources];
	/*ResourceManager::Get().LoadTextureFromFile("textures/white.dds", tex[0], uploadBufferTex[0], cmdListHelper.CmdList());
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
	ASSERT(tex[6] != nullptr);*/
	ResourceManager::Get().LoadTextureFromFile("textures/rock.dds", tex[0], uploadBufferTex[0], cmdListHelper.CmdList());
	ASSERT(tex[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2.dds", tex[1], uploadBufferTex[1], cmdListHelper.CmdList());
	ASSERT(tex[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete.dds", tex[2], uploadBufferTex[2], cmdListHelper.CmdList());
	ASSERT(tex[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor.dds", tex[3], uploadBufferTex[3], cmdListHelper.CmdList());
	ASSERT(tex[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/asphalt.dds", tex[4], uploadBufferTex[4], cmdListHelper.CmdList());
	ASSERT(tex[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/cobblestone2.dds", tex[5], uploadBufferTex[5], cmdListHelper.CmdList());
	ASSERT(tex[5] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/bricks.dds", tex[6], uploadBufferTex[6], cmdListHelper.CmdList());
	ASSERT(tex[6] != nullptr);

	ID3D12Resource* normal[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferNormal[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock_normal.dds", normal[0], uploadBufferNormal[0], cmdListHelper.CmdList());
	ASSERT(normal[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2_normal.dds", normal[1], uploadBufferNormal[1], cmdListHelper.CmdList());
	ASSERT(normal[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete_normal.dds", normal[2], uploadBufferNormal[2], cmdListHelper.CmdList());
	ASSERT(normal[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_normal.dds", normal[3], uploadBufferNormal[3], cmdListHelper.CmdList());
	ASSERT(normal[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/asphalt_normal.dds", normal[4], uploadBufferNormal[4], cmdListHelper.CmdList());
	ASSERT(normal[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/cobblestone2_normal.dds", normal[5], uploadBufferNormal[5], cmdListHelper.CmdList());
	ASSERT(normal[5] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/bricks_normal.dds", normal[6], uploadBufferNormal[6], cmdListHelper.CmdList());
	ASSERT(normal[6] != nullptr);

	ID3D12Resource* height[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferHeight[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock_height.dds", height[0], uploadBufferHeight[0], cmdListHelper.CmdList());
	ASSERT(height[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2_height.dds", height[1], uploadBufferHeight[1], cmdListHelper.CmdList());
	ASSERT(height[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete_height.dds", height[2], uploadBufferHeight[2], cmdListHelper.CmdList());
	ASSERT(height[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_height.dds", height[3], uploadBufferHeight[3], cmdListHelper.CmdList());
	ASSERT(height[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/asphalt_height.dds", height[4], uploadBufferHeight[4], cmdListHelper.CmdList());
	ASSERT(height[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/cobblestone2_height.dds", height[5], uploadBufferHeight[5], cmdListHelper.CmdList());
	ASSERT(height[5] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/bricks_height.dds", height[6], uploadBufferHeight[6], cmdListHelper.CmdList());
	ASSERT(height[6] != nullptr);

	cmdListHelper.ExecuteCmdList();

	tasks.resize(3);	

	BasicCmdListRecorder* basicRecorder{ nullptr };
	GenerateRecorder(cmdListQueue, model1->Meshes(), basicRecorder);
	ASSERT(basicRecorder != nullptr);
	tasks[0].reset(basicRecorder);

	NormalCmdListRecorder* normalRecorder{ nullptr };
	GenerateRecorder(cmdListQueue, model1->Meshes(), tex, normal, normalRecorder);
	ASSERT(normalRecorder != nullptr);
	tasks[1].reset(normalRecorder);

	HeightCmdListRecorder* heightRecorder{ nullptr };
	GenerateRecorder(cmdListQueue, model1->Meshes(), tex, normal, height, heightRecorder);
	ASSERT(heightRecorder != nullptr);
	tasks[2].reset(heightRecorder);
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
	GenerateRecorder(cmdListQueue, geometryBuffers, geometryBuffersCount, recorder);
	ASSERT(recorder != nullptr);
	tasks[0].reset(recorder);
}

