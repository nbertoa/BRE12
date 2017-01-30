#include "HeightScene.h"

#include <tbb/parallel_for.h>

#include <DirectXManager\DirectXManager.h>
#include <GeometryPass/Recorders/HeightCmdListRecorder.h>
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
		// Diffuse
		BRICK2,
		BRICK3,
		COBBLESTONE2,
		COBBLESTONE,
		ROCK3,
		WOOD2,

		// Normal
		BRICK2_NORMAL,
		BRICK3_NORMAL,
		COBBLESTONE2_NORMAL,
		COBBLESTONE_NORMAL,
		ROCK3_NORMAL,
		WOOD2_NORMAL,

		// Height
		BRICK2_HEIGHT,
		BRICK3_HEIGHT,
		COBBLESTONE2_HEIGHT,
		COBBLESTONE_HEIGHT,
		BLACK,
		WOOD2_HEIGHT,

		// Environment
		SKY_BOX,
		DIFFUSE_CUBE_MAP,
		SPECULAR_CUBE_MAP,

		TEXTURES_COUNT
	};

	// Textures to load
	std::vector<std::string> sTexFiles =
	{
		// Diffuse
		"textures/brick/brick2.dds",
		"textures/brick/brick3.dds",
		"textures/cobblestone/cobblestone2.dds",
		"textures/cobblestone/cobblestone.dds",
		"textures/rock/rock3.dds",
		"textures/wood/wood2.dds",

		// Normal
		"textures/brick/brick2_normal.dds",
		"textures/brick/brick3_normal.dds",
		"textures/cobblestone/cobblestone2_normal.dds",
		"textures/cobblestone/cobblestone_normal.dds",
		"textures/rock/rock3_normal.dds",
		"textures/wood/wood2_normal.dds",

		// Height
		"textures/brick/brick2_height.dds",
		"textures/brick/brick3_height.dds",
		"textures/cobblestone/cobblestone2_height.dds",
		"textures/cobblestone/cobblestone_height.dds",
		"textures/black.dds",
		"textures/wood/wood2_height.dds",

		// Environment
		"textures/cubeMaps/milkmill_cube_map.dds",
		"textures/cubeMaps/milkmill_diffuse_cube_map.dds",
		"textures/cubeMaps/milkmill_specular_cube_map.dds",
	};

	enum Models {
		UNREAL,
		MODELS_COUNT
	};

	// Models to load
	std::vector<std::string> sModelFiles =
	{
		"models/unreal.obj",
	};

	const float sS{ 0.05f };
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
		light[0].mPositionAndRange[3] = 100000;
		light[0].mColorAndPower[0] = 1.0f;
		light[0].mColorAndPower[1] = 1.0f;
		light[0].mColorAndPower[2] = 1.0f;
		light[0].mColorAndPower[3] = 100000;

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
		const float scaleFactor,
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

		recorder = new HeightCmdListRecorder();
		
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
		std::vector<ID3D12Resource*> heightsVec;
		heightsVec.resize(numMaterials * numMeshes);

		float tx{ initX };
		float ty{ initY };
		float tz{ initZ };
		for (std::size_t i = 0UL; i < numMaterials; ++i) {
			DirectX::XMFLOAT4X4 w;
			MathUtils::ComputeMatrix(w, tx, ty, tz, scaleFactor, scaleFactor, scaleFactor);

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
			static_cast<std::uint32_t>(materialsVec.size()));
	}
}

void HeightScene::Init() noexcept {
	Scene::Init();

	// Load textures
	sResourceContainer.LoadTextures(sTexFiles, *mCommandAllocators, *mCommandList, *mFence);

	// Load models
	sResourceContainer.LoadModels(sModelFiles, *mCommandAllocators, *mCommandList, *mFence);
}		

void HeightScene::CreateGeometryPassRecorders(
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {

	ASSERT(tasks.empty());
	ASSERT(IsDataValid());

	const std::vector<ID3D12Resource*>& textures = sResourceContainer.GetTextures();
	ASSERT(textures.empty() == false);

	const Model& model = sResourceContainer.GetModel(UNREAL);

	const std::uint32_t numResources{ 6U };

	Material materials[numResources];
	for (std::uint32_t i = 0UL; i < numResources; ++i) {
		materials[i].mBaseColor_MetalMask[3U] = 1.0f;
		materials[i].mSmoothness = 0.9f;
	}

	materials[0].mSmoothness = 0.95f;
	materials[1].mSmoothness = 0.85f;
	materials[2].mSmoothness = 0.75f;
	materials[3].mSmoothness = 0.65f;
	materials[4].mSmoothness = 0.55f;
	materials[5].mSmoothness = 0.45f;

	ID3D12Resource* tex[] =
	{
		textures[BRICK2],
		textures[BRICK3],
		textures[COBBLESTONE2],
		textures[COBBLESTONE],
		textures[ROCK3],
		textures[WOOD2],
	};

	ID3D12Resource* normal[]
	{
		textures[BRICK2_NORMAL],
		textures[BRICK3_NORMAL],
		textures[COBBLESTONE2_NORMAL],
		textures[COBBLESTONE_NORMAL],
		textures[ROCK3_NORMAL],
		textures[WOOD2_NORMAL],
	};

	ID3D12Resource* height[]
	{
		textures[BRICK2_HEIGHT],
		textures[BRICK3_HEIGHT],
		textures[COBBLESTONE2_HEIGHT],
		textures[COBBLESTONE_HEIGHT],
		textures[BLACK],
		textures[WOOD2_HEIGHT],
	};

	ASSERT(numResources == _countof(tex));
	ASSERT(numResources == _countof(normal));
	ASSERT(numResources == _countof(height));
	
	HeightCmdListRecorder* recorder{ nullptr };
	GenerateRecorder(sTx1, sTy1, sTz1, sOffsetX1, 0.0f, 0.0f, sS, model.GetMeshes(), tex, normal, height, materials, numResources, recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));
}

void HeightScene::CreateLightingPassRecorders(
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

void HeightScene::CreateCubeMapResources(
	ID3D12Resource* &skyBoxCubeMap,
	ID3D12Resource* &diffuseIrradianceCubeMap,
	ID3D12Resource* &specularPreConvolvedCubeMap) noexcept
{
	skyBoxCubeMap = &sResourceContainer.GetTexture(SKY_BOX);
	diffuseIrradianceCubeMap = &sResourceContainer.GetTexture(DIFFUSE_CUBE_MAP);
	specularPreConvolvedCubeMap = &sResourceContainer.GetTexture(SPECULAR_CUBE_MAP);
}

