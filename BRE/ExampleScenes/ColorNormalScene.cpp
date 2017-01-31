#include "ColorNormalScene.h"

#include <tbb/parallel_for.h>

#include <DirectXManager\DirectXManager.h>
#include <GeometryPass/Recorders/ColorNormalCmdListRecorder.h>
#include <LightingPass/PunctualLight.h>
#include <LightingPass/Recorders/PunctualLightCmdListRecorder.h>
#include <MaterialManager/Material.h>
#include <MathUtils\MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Scene/SceneUtils.h>

namespace {
	SceneUtils::SceneResources sResourceContainer;

	enum Textures {
		// Normal
		ROCK_NORMAL,
		ROCK2_NORMAL,
		WOOD_NORMAL,
		FLOOR_NORMAL,
		SAND_NORMAL,
		COBBLESTONE_NORMAL,

		// Environment
		SKY_BOX,
		DIFFUSE_CUBE_MAP,
		SPECULAR_CUBE_MAP,

		TEXTURES_COUNT
	};

	// Textures to load
	std::vector<std::string> sTexFiles =
	{

		// Normal
		"textures/rock/rock_normal.dds",
		"textures/rock/rock2_normal.dds",
		"textures/wood/wood_normal.dds",
		"textures/floor_normal.dds",
		"textures/sand/sand_normal.dds",
		"textures/cobblestone/cobblestone_normal.dds",

		// Environment
		"textures/cubeMaps/milkmill_cube_map.dds",
		"textures/cubeMaps/milkmill_diffuse_cube_map.dds",
		"textures/cubeMaps/milkmill_specular_cube_map.dds",
	};

	enum Models {
		MITSUBA_FLOOR,
		MODELS_COUNT
	};

	// Models to load
	std::vector<std::string> sModelFiles =
	{
		"models/mitsubaFloor.obj",
	};

	const float sS{ 2.0f };

	const float sTx1{ 0.0f };
	const float sTy1{ -3.5f };
	const float sTz1{ 30.0f };
	const float sOffsetX1{ 25.0f };

	void GenerateRecorder(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		PunctualLightCmdListRecorder* &recorder) {
		recorder = new PunctualLightCmdListRecorder();
		PunctualLight light[1];
		light[0].mPositionAndRange[0] = 0.0f;
		light[0].mPositionAndRange[1] = 300.0f;
		light[0].mPositionAndRange[2] = -100.0f;
		light[0].mPositionAndRange[3] = 10000.0f;
		light[0].mColorAndPower[0] = 1.0f;
		light[0].mColorAndPower[1] = 1.0f;
		light[0].mColorAndPower[2] = 1.0f;
		light[0].mColorAndPower[3] = 1000000.0f;

		recorder->Init(
			geometryBuffers,
			geometryBuffersCount,
			depthBuffer,
			light,
			_countof(light));
	}

	void GenerateRecorder(
		const float initX,
		const float initY,
		const float initZ,
		const float offsetX,
		const float offsetY,
		const float offsetZ,
		const std::vector<Mesh>& meshes,
		ID3D12Resource** normals,
		Material* materials,
		const std::size_t numMaterials,
		ColorNormalCmdListRecorder* &recorder) {

		ASSERT(normals != nullptr);

		recorder = new ColorNormalCmdListRecorder();

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
		}

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
			MathUtils::ComputeMatrix(w, tx, ty, tz, sS, sS, sS);

			Material& mat(materials[i]);
			ID3D12Resource* normal{ normals[i] };
			for (std::size_t j = 0UL; j < numMeshes; ++j) {
				const std::size_t index{ i + j * numMaterials };
				materialsVec[index] = mat;
				normalsVec[index] = normal;
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
			normalsVec.data(),
			static_cast<std::uint32_t>(materialsVec.size()));
	}
}

void ColorNormalScene::Init() noexcept {
	Scene::Init();

	// Load textures
	sResourceContainer.LoadTextures(sTexFiles, *mCommandAllocator, *mCommandList);

	// Load models
	sResourceContainer.LoadModels(sModelFiles, *mCommandAllocator, *mCommandList);
}

void ColorNormalScene::CreateGeometryPassRecorders(
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {

	ASSERT(tasks.empty());
	ASSERT(IsDataValid());

	const std::vector<ID3D12Resource*>& textures = sResourceContainer.GetTextures();
	ASSERT(textures.empty() == false);

	const Model& model = sResourceContainer.GetModel(MITSUBA_FLOOR);

	const std::uint32_t numResources{ 6U };

	Material materials[numResources];
	for (std::uint32_t i = 0UL; i < numResources; ++i) {
		materials[i].RandomizeBaseColor();
		materials[i].mBaseColor_MetalMask[3U] = 0.0f;
		materials[i].mSmoothness = 0.7f;
	}

	ID3D12Resource* normal[]
	{
		textures[ROCK_NORMAL],
		textures[ROCK2_NORMAL],
		textures[WOOD_NORMAL],
		textures[FLOOR_NORMAL],
		textures[SAND_NORMAL],
		textures[COBBLESTONE_NORMAL],
	};
	ASSERT(numResources == _countof(normal));

	ColorNormalCmdListRecorder* recorder{ nullptr };
	GenerateRecorder(sTx1, sTy1, sTz1, sOffsetX1, 0.0f, 0.0f, model.GetMeshes(), normal, materials, numResources, recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));
}

void ColorNormalScene::CreateLightingPassRecorders(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	std::vector<std::unique_ptr<LightingPassCmdListRecorder>>& tasks) noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	ASSERT(IsDataValid());

	tasks.resize(1UL);
	PunctualLightCmdListRecorder* recorder{ nullptr };
	GenerateRecorder(
		geometryBuffers,
		geometryBuffersCount,
		depthBuffer,
		recorder);
	ASSERT(recorder != nullptr);
	tasks[0].reset(recorder);
}

void ColorNormalScene::CreateCubeMapResources(
	ID3D12Resource* &skyBoxCubeMap,
	ID3D12Resource* &diffuseIrradianceCubeMap,
	ID3D12Resource* &specularPreConvolvedCubeMap) noexcept
{
	skyBoxCubeMap = &sResourceContainer.GetTexture(SKY_BOX);
	diffuseIrradianceCubeMap = &sResourceContainer.GetTexture(DIFFUSE_CUBE_MAP);
	specularPreConvolvedCubeMap = &sResourceContainer.GetTexture(SPECULAR_CUBE_MAP);
}

