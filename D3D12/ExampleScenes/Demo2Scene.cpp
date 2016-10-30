#include "Demo2Scene.h"

#include <tbb/parallel_for.h>

#include <ExampleScenes/SceneUtils.h>
#include <GeometryPass/Recorders/ColorCmdListRecorder.h>
#include <GeometryPass/Recorders/NormalCmdListRecorder.h>
#include <GlobalData/D3dData.h>
#include <Material/Material.h>
#include <MathUtils\MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>

namespace {
	enum Textures {
		// Metal
		METAL = 0U,
		METAL_NORMAL,
		METAL2,
		METAL2_NORMAL,
		METAL3,
		METAL3_NORMAL,

		// Brick
		BRICK,
		BRICK_HEIGHT,
		BRICK_NORMAL,
		BRICK2,
		BRICK2_HEIGHT,
		BRICK2_NORMAL,
		BRICK3,
		BRICK3_HEIGHT,
		BRICK3_NORMAL,

		// Wood
		WOOD,
		WOOD_HEIGHT,
		WOOD_NORMAL,

		// Rock
		ROCK,
		ROCK_HEIGHT,
		ROCK_NORMAL,
		ROCK2,
		ROCK2_HEIGHT,
		ROCK2_NORMAL,
		ROCK3,
		ROCK3_NORMAL,
		ROCK4,
		ROCK4_NORMAL,
		ROCK5,
		ROCK5_NORMAL,
		ROCK6,
		ROCK6_HEIGHT,
		ROCK6_NORMAL,

		// Cobblestone
		COBBLESTONE,
		COBBLESTONE_HEIGHT,
		COBBLESTONE_NORMAL,
		COBBLESTONE2,
		COBBLESTONE2_HEIGHT,
		COBBLESTONE2_NORMAL,

		TEXTURES_COUNT
	};

	void LoadTextures(
		ID3D12CommandQueue& cmdQueue,
		ID3D12CommandAllocator& cmdAlloc,
		ID3D12GraphicsCommandList& cmdList,
		ID3D12Fence& fence,
		std::vector<ID3D12Resource*>& textures) noexcept 
	{
		std::vector<std::string> texFiles = 
		{
			// Metal
			"textures/metal/metal.dds",
			"textures/metal/metal_normal.dds",
			"textures/metal/metal2.dds",
			"textures/metal/metal2_normal.dds",
			"textures/metal/metal3.dds",
			"textures/metal/metal3_normal.dds",

			// Brick
			"textures/brick/brick.dds",
			"textures/brick/brick_height.dds",
			"textures/brick/brick_normal.dds",
			"textures/brick/brick2.dds",
			"textures/brick/brick2_height.dds",
			"textures/brick/brick2_normal.dds",
			"textures/brick/brick3.dds",
			"textures/brick/brick3_height.dds",
			"textures/brick/brick3_normal.dds",

			// Wood
			"textures/wood/wood.dds",
			"textures/wood/wood_height.dds",
			"textures/wood/wood_normal.dds",

			// Rock
			"textures/rock/rock.dds",
			"textures/rock/rock_height.dds",
			"textures/rock/rock_normal.dds",
			"textures/rock/rock2.dds",
			"textures/rock/rock2_height.dds",
			"textures/rock/rock2_normal.dds",
			"textures/rock/rock3.dds",
			"textures/rock/rock3_normal.dds",
			"textures/rock/rock4.dds",
			"textures/rock/rock4_normal.dds",
			"textures/rock/rock5.dds",
			"textures/rock/rock5_normal.dds",
			"textures/rock/rock6.dds",
			"textures/rock/rock6_height.dds",
			"textures/rock/rock6_normal.dds",

			// Cobblestone
			"textures/cobblestone/cobblestone.dds",
			"textures/cobblestone/cobblestone_height.dds",
			"textures/cobblestone/cobblestone_normal.dds",
			"textures/cobblestone/cobblestone2.dds",
			"textures/cobblestone/cobblestone2_height.dds",
			"textures/cobblestone/cobblestone2_normal.dds",
		};

		ASSERT(texFiles.size() == TEXTURES_COUNT);

		SceneUtils::LoadTextures(texFiles, cmdQueue, cmdAlloc, cmdList, fence, textures);

		ASSERT(textures.size() == texFiles.size());
	}

	const char* sSkyBoxFile{ "textures/cubeMaps/milkmill_cube_map.dds" };
	const char* sDiffuseEnvironmentFile{ "textures/cubeMaps/milkmill_diffuse_cube_map.dds" };
	const char* sSpecularEnvironmentFile{ "textures/cubeMaps/milkmill_specular_cube_map.dds" };

	const float sFloorScale{ 1.0f };
	const float sFloorTx{ -150.0f };
	const float sFloorTy{ -20.5f };
	const float sFloorTz{ 150.0f };

	const float sModel{ 0.05f };
	const float sSphere{ 3.0f };
	
	const float sTx1{ 0.0f };
	const float sTy1{ sFloorTy };
	const float sTz1{ 30.0f };
	const float sOffsetX1{ 20.0f };

	const float sTx2{ 0.0f };
	const float sTy2{ sFloorTy };
	const float sTz2{ 80.0f };
	const float sOffsetX2{ 20.0f };

	const float sTx3{ 0.0f };
	const float sTy3{ sFloorTy };
	const float sTz3{ 130.0f };
	const float sOffsetX3{ 20.0f };

	const float sTx4{ 0.0f };
	const float sTy4{ sFloorTy };
	const float sTz4{ -20.0f };
	const float sOffsetX4{ 20.0f };

	const float sTx5{ 0.0f };
	const float sTy5{ -8.0f };
	const float sTz5{ -70.0f };
	const float sOffsetX5{ 20.0f };

	const float sTx6{ 0.0f };
	const float sTy6{ sFloorTy + 3.0f };
	const float sTz6{ -130.0f };
	const float sOffsetX6{ 30.0f };

	void GenerateRecorder(
		const float initX,
		const float initY,
		const float initZ,
		const float offsetX,
		const float offsetY,
		const float offsetZ,
		const float scaleFactor,
		const std::vector<Mesh>& meshes,
		ID3D12Resource** textures,
		ID3D12Resource** normals,
		Material* materials,
		const std::size_t numMaterials,
		NormalCmdListRecorder* &recorder) {

		ASSERT(textures != nullptr);
		ASSERT(normals != nullptr);

		// Fill geometry data
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

		// Fill material and textures
		std::vector<Material> materialsVec;
		materialsVec.resize(numMaterials * numMeshes);
		std::vector<ID3D12Resource*> texturesVec;
		texturesVec.resize(numMaterials * numMeshes);
		std::vector<ID3D12Resource*> normalsVec;
		normalsVec.resize(numMaterials * numMeshes);

		float tx{ initX };
		float ty{ initY };
		float tz{ initZ };
		for (std::size_t i = 0UL; i < numMaterials; ++i) {
			DirectX::XMFLOAT4X4 w;
			MathUtils::ComputeMatrix(w, tx, ty, tz, scaleFactor, scaleFactor, scaleFactor);

			Material& mat(materials[i]);
			ID3D12Resource* texture{ textures[i] };
			ID3D12Resource* normal{ normals[i] };
			for (std::size_t j = 0UL; j < numMeshes; ++j) {
				const std::size_t index{ i + j * numMaterials };
				materialsVec[index] = mat;
				texturesVec[index] = texture;
				normalsVec[index] = normal;
				GeometryPassCmdListRecorder::GeometryData& geomData{ geomDataVec[j] };
				geomData.mWorldMatrices.push_back(w);
			}

			tx += offsetX;
			ty += offsetY;
			tz += offsetZ;
		}

		// Create recorder
		recorder = new NormalCmdListRecorder(D3dData::Device());
		recorder->Init(
			geomDataVec.data(), 
			static_cast<std::uint32_t>(geomDataVec.size()), 
			materialsVec.data(), 
			texturesVec.data(), 
			normalsVec.data(), 
			static_cast<std::uint32_t>(materialsVec.size()));
	}

	void GenerateRecorder(
		const float initX,
		const float initY,
		const float initZ,
		const float offsetX,
		const float offsetY,
		const float offsetZ,
		const float scaleFactor,
		const std::vector<Mesh>& meshes,
		Material* materials,
		const std::size_t numMaterials,
		ColorCmdListRecorder* &recorder) {

		// Fill geometry data
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

		// Fill material and textures
		std::vector<Material> materialsVec;
		materialsVec.resize(numMaterials * numMeshes);

		float tx{ initX };
		float ty{ initY };
		float tz{ initZ };
		for (std::size_t i = 0UL; i < numMaterials; ++i) {
			DirectX::XMFLOAT4X4 w;
			MathUtils::ComputeMatrix(w, tx, ty, tz, scaleFactor, scaleFactor, scaleFactor);

			Material& mat(materials[i]);
			for (std::size_t j = 0UL; j < numMeshes; ++j) {
				const std::size_t index{ i + j * numMaterials };
				materialsVec[index] = mat;
				GeometryPassCmdListRecorder::GeometryData& geomData{ geomDataVec[j] };
				geomData.mWorldMatrices.push_back(w);
			}

			tx += offsetX;
			ty += offsetY;
			tz += offsetZ;
		}

		// Create recorder
		recorder = new ColorCmdListRecorder(D3dData::Device());
		recorder->Init(
			geomDataVec.data(),
			static_cast<std::uint32_t>(geomDataVec.size()),
			materialsVec.data(),
			static_cast<std::uint32_t>(materialsVec.size()));
	}

	void GenerateFloorRecorder(
		const std::vector<Mesh>& meshes,
		ID3D12Resource* texture,
		ID3D12Resource* normal,
		NormalCmdListRecorder* &recorder) {

		ASSERT(texture != nullptr);
		ASSERT(normal != nullptr);

		// Compute world matrix
		DirectX::XMFLOAT4X4 w;
		MathUtils::ComputeMatrix(w, sFloorTx, sFloorTy, sFloorTz, sFloorScale, sFloorScale, sFloorScale);

		const std::size_t numMeshes{ meshes.size() };
		ASSERT(numMeshes > 0UL);
		
		// Fill geometry data
		std::vector<GeometryPassCmdListRecorder::GeometryData> geomDataVec;
		geomDataVec.resize(numMeshes);
		for (std::size_t i = 0UL; i < numMeshes; ++i) {
			GeometryPassCmdListRecorder::GeometryData& geomData{ geomDataVec[i] };
			const Mesh& mesh{ meshes[i] };

			geomData.mVertexBufferData = mesh.VertexBufferData();
			geomData.mIndexBufferData = mesh.IndexBufferData();
			
			geomData.mWorldMatrices.push_back(w);
		}

		// Fill material
		Material material{ 1.0f, 1.0f, 1.0f, 0.0f, 0.85f };

		// Build recorder
		recorder = new NormalCmdListRecorder(D3dData::Device());
		recorder->Init(
			geomDataVec.data(),
			static_cast<std::uint32_t>(geomDataVec.size()),
			&material,
			&texture,
			&normal,
			1);
	}
}

void Demo2Scene::GenerateGeomPassRecorders(
	ID3D12CommandQueue& cmdQueue,
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {

	ASSERT(tasks.empty());
	ASSERT(ValidateData());

	std::vector<ID3D12Resource*> textures;

	LoadTextures(cmdQueue, *mCmdAlloc, *mCmdList, *mFence, textures);

	CHECK_HR(mCmdList->Reset(mCmdAlloc, nullptr));

	Model* sphere;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBufferSphere;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBufferSphere;
	ModelManager::Get().CreateSphere(2, 50, 50, sphere, *mCmdList, uploadVertexBufferSphere, uploadIndexBufferSphere);
	ASSERT(sphere != nullptr);

	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBufferModel;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBufferModel;
	ModelManager::Get().LoadModel("models/unreal.obj", model, *mCmdList, uploadVertexBufferModel, uploadIndexBufferModel);
	ASSERT(model != nullptr);

	Model* bunny;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBufferBunny;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBufferBunny;
	ModelManager::Get().LoadModel("models/bunny.obj", bunny, *mCmdList, uploadVertexBufferBunny, uploadIndexBufferBunny);
	ASSERT(bunny != nullptr);

	Model* buddha;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBufferBuddha;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBufferBuddha;
	ModelManager::Get().LoadModel("models/buddha.obj", buddha, *mCmdList, uploadVertexBufferBuddha, uploadIndexBufferBuddha);
	ASSERT(buddha != nullptr);

	Model* floor;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBufferFloor;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBufferFloor;
	ModelManager::Get().LoadModel("models/floor.obj", floor, *mCmdList, uploadVertexBufferFloor, uploadIndexBufferFloor);
	ASSERT(floor != nullptr);

	ExecuteCommandList(cmdQueue);

	//
	// Generate floor
	//
	NormalCmdListRecorder* normalRecorder{ nullptr };
	GenerateFloorRecorder(
		floor->Meshes(),
		textures[WOOD],
		textures[WOOD_NORMAL],
		normalRecorder);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(normalRecorder));

	//
	// Generate metals
	//	
	std::vector<Material> materials = 
	{
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.95f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.85f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.75f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.95f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.85f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.75f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.95f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.85f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.75f },
	};	

	std::vector<ID3D12Resource*> diffuses =
	{
		textures[METAL],
		textures[METAL],
		textures[METAL],
		textures[METAL2],
		textures[METAL2],
		textures[METAL2],
		textures[METAL3],
		textures[METAL3],
		textures[METAL3],
	};

	std::vector<ID3D12Resource*> normals =
	{
		textures[METAL_NORMAL],
		textures[METAL_NORMAL],
		textures[METAL_NORMAL],
		textures[METAL2_NORMAL],
		textures[METAL2_NORMAL],
		textures[METAL2_NORMAL],
		textures[METAL3_NORMAL],
		textures[METAL3_NORMAL],
		textures[METAL3_NORMAL],
	};	

	ASSERT(materials.size() == diffuses.size());
	ASSERT(normals.size() == diffuses.size());
	
	GenerateRecorder(
		sTx2, 
		sTy2, 
		sTz2, 
		sOffsetX2, 
		0.0f, 
		0.0f, 
		sModel, 
		model->Meshes(), 
		diffuses.data(), 
		normals.data(), 
		materials.data(), 
		materials.size(),
		normalRecorder);
	ASSERT(normalRecorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(normalRecorder));
	
	//
	// Generate wood
	//

	materials =
	{
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.95f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.75f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.55f },
	};

	diffuses =
	{
		textures[WOOD],
		textures[WOOD],
		textures[WOOD],
	};

	normals =
	{
		textures[WOOD_NORMAL],
		textures[WOOD_NORMAL],
		textures[WOOD_NORMAL],
	};

	ASSERT(materials.size() == diffuses.size());
	ASSERT(normals.size() == diffuses.size());

	GenerateRecorder(
		sTx3,
		sTy3,
		sTz3,
		sOffsetX3,
		0.0f,
		0.0f,
		sModel,
		model->Meshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		normalRecorder);
	ASSERT(normalRecorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(normalRecorder));

	//
	// Generate rocks, bricks and cobblestones (metals)
	//

	materials =
	{
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.8f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.8f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.8f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.8f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.8f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.8f },
		{ 1.0f, 0.5f, 1.0f, 1.0f, 0.8f },
		{ 1.0f, 1.0f, 0.5f, 1.0f, 0.8f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.8f },
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.8f },
		{ 1.0f, 0.7f, 0.4f, 1.0f, 0.8f },
	};

	diffuses =
	{
		textures[ROCK],
		textures[ROCK2],
		textures[ROCK3],
		textures[ROCK4],
		textures[ROCK5],
		textures[ROCK6],
		textures[BRICK],
		textures[BRICK2],
		textures[BRICK3],
		textures[COBBLESTONE],
		textures[COBBLESTONE2],
	};

	normals =
	{
		textures[ROCK_NORMAL],
		textures[ROCK2_NORMAL],
		textures[ROCK3_NORMAL],
		textures[ROCK4_NORMAL],
		textures[ROCK5_NORMAL],
		textures[ROCK6_NORMAL],
		textures[BRICK_NORMAL],
		textures[BRICK2_NORMAL],
		textures[BRICK3_NORMAL],
		textures[COBBLESTONE_NORMAL],
		textures[COBBLESTONE2_NORMAL],
	};

	ASSERT(materials.size() == diffuses.size());
	ASSERT(normals.size() == diffuses.size());

	GenerateRecorder(
		sTx1,
		sTy1,
		sTz1,
		sOffsetX1,
		0.0f,
		0.0f,
		sModel,
		model->Meshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		normalRecorder);
	ASSERT(normalRecorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(normalRecorder));

	//
	// Generate rocks, bricks and cobblestones (non-metals)
	//

	materials =
	{
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.85f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.85f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.85f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.85f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.85f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.85f },
		{ 1.0f, 0.5f, 1.0f, 0.0f, 0.75f },
		{ 1.0f, 1.0f, 0.5f, 0.0f, 0.75f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.75f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.75f },
		{ 1.0f, 0.7f, 0.4f, 0.0f, 0.75f },
	};

	diffuses =
	{
		textures[ROCK],
		textures[ROCK2],
		textures[ROCK3],
		textures[ROCK4],
		textures[ROCK5],
		textures[ROCK6],
		textures[BRICK],
		textures[BRICK2],
		textures[BRICK3],
		textures[COBBLESTONE],
		textures[COBBLESTONE2],
	};

	normals =
	{
		textures[ROCK_NORMAL],
		textures[ROCK2_NORMAL],
		textures[ROCK3_NORMAL],
		textures[ROCK4_NORMAL],
		textures[ROCK5_NORMAL],
		textures[ROCK6_NORMAL],
		textures[BRICK_NORMAL],
		textures[BRICK2_NORMAL],
		textures[BRICK3_NORMAL],
		textures[COBBLESTONE_NORMAL],
		textures[COBBLESTONE2_NORMAL],
	};

	ASSERT(materials.size() == diffuses.size());
	ASSERT(normals.size() == diffuses.size());

	GenerateRecorder(
		sTx4,
		sTy4,
		sTz4,
		sOffsetX4,
		0.0f,
		0.0f,
		sModel,
		model->Meshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		normalRecorder);
	ASSERT(normalRecorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(normalRecorder));

	//
	// Generate color mapping
	//

	materials =
	{
		{ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f, 0.85f },
		{ 0.0f, 0.0f, 0.0f, 0.0f, 0.75f },
	};

	ColorCmdListRecorder* colorRecorder;
	GenerateRecorder(
		sTx5,
		sTy5,
		sTz5,
		sOffsetX5,
		0.0f,
		0.0f,
		25,
		buddha->Meshes(),
		materials.data(),
		materials.size(),
		colorRecorder);
	ASSERT(colorRecorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(colorRecorder));

	//
	// Generate color mapping
	//

	materials =
	{
		{ 0.7f, 0.7f, 0.3f, 1.0f, 0.85f },
		{ 0.3f, 0.7f, 0.7f, 1.0f, 0.85f },
		{ 0.7f, 0.3f, 0.7f, 1.0f, 0.85f },
	};

	GenerateRecorder(
		sTx6,
		sTy6,
		sTz6,
		sOffsetX6,
		0.0f,
		0.0f,
		8,
		bunny->Meshes(),
		materials.data(),
		materials.size(),
		colorRecorder);
	ASSERT(colorRecorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(colorRecorder));
}

void Demo2Scene::GenerateLightingPassRecorders(
	Microsoft::WRL::ComPtr<ID3D12Resource>*,
	const std::uint32_t,
	ID3D12Resource&,
	std::vector<std::unique_ptr<LightingPassCmdListRecorder>>&) noexcept
{
}

void Demo2Scene::GenerateCubeMaps(
	ID3D12CommandQueue& cmdQueue,
	ID3D12Resource* &skyBoxCubeMap,
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

	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex3;
	ResourceManager::Get().LoadTextureFromFile(sSkyBoxFile, skyBoxCubeMap, uploadBufferTex3, *mCmdList);
	ASSERT(skyBoxCubeMap != nullptr);

	ExecuteCommandList(cmdQueue);
}

