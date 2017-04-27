#include "AmbientOcclussionScene.h"

#include <tbb/parallel_for.h>

#include <DirectXManager\DirectXManager.h>
#include <GeometryPass/Recorders/NormalCmdListRecorder.h>
#include <MaterialManager/Material.h>
#include <MathUtils\MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Scene/SceneUtils.h>

using namespace DirectX;

namespace {
	SceneUtils::SceneResources sResourceContainer;

	enum Textures {
		// Brick
		BRICK = 0U,
		BRICK_NORMAL,

		// Wood
		WOOD,
		WOOD_NORMAL,

		// Environment
		SKY_BOX,
		DIFFUSE_CUBE_MAP,
		SPECULAR_CUBE_MAP,

		TEXTURES_COUNT
	};

	// Textures to load
	std::vector<std::string> sTexFiles =
	{
		// Brick
		"resources/textures/brick/brick2.dds",
		"resources/textures/brick/brick2_normal.dds",

		// Wood
		"resources/textures/wood/wood.dds",
		"resources/textures/wood/wood_normal.dds",

		// Environment
		"resources/textures/cubeMaps/milkmill_cube_map.dds",
		"resources/textures/cubeMaps/milkmill_diffuse_cube_map.dds",
		"resources/textures/cubeMaps/milkmill_specular_cube_map.dds",
	};

	enum Models {
		UNREAL,
		FLOOR,

		MODELS_COUNT
	};

	// Models to load
	std::vector<std::string> sModelFiles =
	{
		"resources/models/mitsubaSphere.obj",
		"resources/models/floor.obj",
	};

	const float sFloorScale{ 1.0f };
	const float sFloorTx{ -150.0f };
	const float sFloorTy{ -20.5f };
	const float sFloorTz{ 150.0f };

	const float sModel{ 0.20f };

	const float sTx{ 0.0f };
	const float sTy{ sFloorTy + 0.5f };
	const float sOffsetX{ 13.5f };

	const float sTz0{ 0.0f };
	const float sTz1{ -13.0f };
	const float sTz2{ -26.0f };
	const float sTz3{ -39.0f };
	const float sTz4{ -52.0f };
	const float sTz5{ -65.0f };
	const float sTz6{ -78.0f };
	const float sTz7{ -91.0f };
	const float sTz8{ -104.0f };
	const float sTz9{ -117.0f };

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
			geomData.mVertexBufferData = mesh.GetVertexBufferData();
			geomData.mIndexBufferData = mesh.GetIndexBufferData();
			geomData.mWorldMatrices.reserve(numMaterials);
			geomData.mInverseTransposeWorldMatrices.reserve(numMaterials);
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
			XMFLOAT4X4 worldMatrix;
			MathUtils::ComputeMatrix(worldMatrix, tx, ty, tz, scaleFactor, scaleFactor, scaleFactor);

			XMFLOAT4X4 inverseTransposeWorldMatrix;
			MathUtils::StoreInverseTransposeMatrix(worldMatrix, inverseTransposeWorldMatrix);

			Material& mat(materials[i]);
			ID3D12Resource* texture{ textures[i] };
			ID3D12Resource* normal{ normals[i] };
			for (std::size_t j = 0UL; j < numMeshes; ++j) {
				const std::size_t index{ i + j * numMaterials };
				materialsVec[index] = mat;
				texturesVec[index] = texture;
				normalsVec[index] = normal;
				GeometryPassCmdListRecorder::GeometryData& geomData{ geomDataVec[j] };
				geomData.mWorldMatrices.push_back(worldMatrix);
				geomData.mInverseTransposeWorldMatrices.push_back(inverseTransposeWorldMatrix);
			}

			tx += offsetX;
			ty += offsetY;
			tz += offsetZ;
		}

		// Create recorder
		recorder = new NormalCmdListRecorder();
		recorder->Init(
			geomDataVec.data(),
			static_cast<std::uint32_t>(geomDataVec.size()),
			materialsVec.data(),
			texturesVec.data(),
			normalsVec.data(),
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
		XMFLOAT4X4 w;
		MathUtils::ComputeMatrix(w, sFloorTx, sFloorTy, sFloorTz, sFloorScale, sFloorScale, sFloorScale);

		const std::size_t numMeshes{ meshes.size() };
		ASSERT(numMeshes > 0UL);

		// Fill geometry data
		std::vector<GeometryPassCmdListRecorder::GeometryData> geomDataVec;
		geomDataVec.resize(numMeshes);
		for (std::size_t i = 0UL; i < numMeshes; ++i) {
			GeometryPassCmdListRecorder::GeometryData& geomData{ geomDataVec[i] };
			const Mesh& mesh{ meshes[i] };

			geomData.mVertexBufferData = mesh.GetVertexBufferData();
			geomData.mIndexBufferData = mesh.GetIndexBufferData();

			geomData.mWorldMatrices.push_back(w);
		}

		// Fill material
		Material material{ 1.0f, 1.0f, 1.0f, 0.0f, 0.85f };

		// Build recorder
		recorder = new NormalCmdListRecorder();
		recorder->Init(
			geomDataVec.data(),
			static_cast<std::uint32_t>(geomDataVec.size()),
			&material,
			&texture,
			&normal,
			1);
	}
}

void AmbientOcclussionScene::Init() noexcept {
	Scene::Init();

	// Load textures
	sResourceContainer.LoadTextures(sTexFiles, *mCommandAllocator, *mCommandList);

	// Load models
	sResourceContainer.LoadModels(sModelFiles, *mCommandAllocator, *mCommandList);
}

void AmbientOcclussionScene::CreateGeometryPassRecorders(
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {

	ASSERT(tasks.empty());
	ASSERT(IsDataValid());

	const std::vector<ID3D12Resource*>& textures = sResourceContainer.GetTextures();
	ASSERT(textures.empty() == false);
	const Model& model = sResourceContainer.GetModel(UNREAL);
	const Model& floor = sResourceContainer.GetModel(FLOOR);

	//
	// Generate floor
	//
	NormalCmdListRecorder* recorder{ nullptr };
	NormalCmdListRecorder* recorder2{ nullptr };
	GenerateFloorRecorder(
		floor.GetMeshes(),
		textures[WOOD],
		textures[WOOD_NORMAL],
		recorder2);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder2));

	//
	// Generate metals
	//	
	std::vector<Material> materials =
	{
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.95f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.85f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.75f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.65f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.55f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.45f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.35f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.25f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.15f },
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.05f },
	};

	std::vector<ID3D12Resource*> diffuses =
	{
		textures[BRICK],
		textures[BRICK],
		textures[BRICK],
		textures[BRICK],
		textures[BRICK],
		textures[BRICK],
		textures[BRICK],
		textures[BRICK],
		textures[BRICK],
		textures[BRICK],
	};

	std::vector<ID3D12Resource*> normals =
	{
		textures[BRICK_NORMAL],
		textures[BRICK_NORMAL],
		textures[BRICK_NORMAL],
		textures[BRICK_NORMAL],
		textures[BRICK_NORMAL],
		textures[BRICK_NORMAL],
		textures[BRICK_NORMAL],
		textures[BRICK_NORMAL],
		textures[BRICK_NORMAL],
		textures[BRICK_NORMAL],
	};

	ASSERT(materials.size() == diffuses.size());
	ASSERT(normals.size() == diffuses.size());

	GenerateRecorder(
		sTx,
		sTy,
		sTz0,
		sOffsetX,
		0.0f,
		0.0f,
		sModel,
		model.GetMeshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));

	GenerateRecorder(
		sTx,
		sTy,
		sTz1,
		sOffsetX,
		0.0f,
		0.0f,
		sModel,
		model.GetMeshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));

	GenerateRecorder(
		sTx,
		sTy,
		sTz2,
		sOffsetX,
		0.0f,
		0.0f,
		sModel,
		model.GetMeshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));

	GenerateRecorder(
		sTx,
		sTy,
		sTz3,
		sOffsetX,
		0.0f,
		0.0f,
		sModel,
		model.GetMeshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));

	GenerateRecorder(
		sTx,
		sTy,
		sTz4,
		sOffsetX,
		0.0f,
		0.0f,
		sModel,
		model.GetMeshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));

	GenerateRecorder(
		sTx,
		sTy,
		sTz5,
		sOffsetX,
		0.0f,
		0.0f,
		sModel,
		model.GetMeshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));

	GenerateRecorder(
		sTx,
		sTy,
		sTz6,
		sOffsetX,
		0.0f,
		0.0f,
		sModel,
		model.GetMeshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));

	GenerateRecorder(
		sTx,
		sTy,
		sTz7,
		sOffsetX,
		0.0f,
		0.0f,
		sModel,
		model.GetMeshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));

	GenerateRecorder(
		sTx,
		sTy,
		sTz8,
		sOffsetX,
		0.0f,
		0.0f,
		sModel,
		model.GetMeshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));

	GenerateRecorder(
		sTx,
		sTy,
		sTz9,
		sOffsetX,
		0.0f,
		0.0f,
		sModel,
		model.GetMeshes(),
		diffuses.data(),
		normals.data(),
		materials.data(),
		materials.size(),
		recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));
}

void AmbientOcclussionScene::CreateLightingPassRecorders(
	Microsoft::WRL::ComPtr<ID3D12Resource>*,
	const std::uint32_t,
	ID3D12Resource&,
	std::vector<std::unique_ptr<LightingPassCmdListRecorder>>&) noexcept
{
}

void AmbientOcclussionScene::CreateIndirectLightingResources(
	ID3D12Resource* &skyBoxCubeMap,
	ID3D12Resource* &diffuseIrradianceCubeMap,
	ID3D12Resource* &specularPreConvolvedCubeMap) noexcept
{
	skyBoxCubeMap = &sResourceContainer.GetTexture(SKY_BOX);
	diffuseIrradianceCubeMap = &sResourceContainer.GetTexture(DIFFUSE_CUBE_MAP);
	specularPreConvolvedCubeMap = &sResourceContainer.GetTexture(SPECULAR_CUBE_MAP);
}

