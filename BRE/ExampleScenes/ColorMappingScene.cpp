#include "ColorMappingScene.h"

#include <tbb/parallel_for.h>

#include <GeometryPass/Recorders/ColorCmdListRecorder.h>
#include <GlobalData/D3dData.h>
#include <LightingPass/PunctualLight.h>
#include <LightingPass/Recorders/PunctualLightCmdListRecorder.h>
#include <Material/Material.h>
#include <MathUtils\MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Scene/SceneUtils.h>

namespace {
	SceneUtils::ResourceContainer sResourceContainer;

	enum Textures {
		// Environment
		SKY_BOX,
		DIFFUSE_CUBE_MAP,
		SPECULAR_CUBE_MAP,

		TEXTURES_COUNT
	};

	// Textures to load
	std::vector<std::string> sTexFiles =
	{
		// Environment
		"textures/cubeMaps/milkmill_cube_map.dds",
		"textures/cubeMaps/milkmill_diffuse_cube_map.dds",
		"textures/cubeMaps/milkmill_specular_cube_map.dds",
	};

	enum Models {
		BUNNY,
		MODELS_COUNT
	};

	// Models to load
	std::vector<std::string> sModelFiles =
	{
		"models/character1.obj",
	};

	const float sS{ 0.10f };

	const float sTx{ 0.0f };
	const float sTy{ -3.5f };
	const float sTz{ 25.0f };	
	const float sOffsetX{ 15.0f };

	void GenerateRecorder(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		PunctualLightCmdListRecorder* &recorder) {
		recorder = new PunctualLightCmdListRecorder();
		PunctualLight light[1];
		light[0].mPosAndRange[0] = 0.0f;
		light[0].mPosAndRange[1] = 300.0f;
		light[0].mPosAndRange[2] = -100.0f;
		light[0].mPosAndRange[3] = 5000.0f;
		light[0].mColorAndPower[0] = 1.0f;
		light[0].mColorAndPower[1] = 1.0f;
		light[0].mColorAndPower[2] = 1.0f;
		light[0].mColorAndPower[3] = 10000000.0f;

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
		ColorCmdListRecorder* &recorder) {
		recorder = new ColorCmdListRecorder();

		const std::size_t numMaterials(Materials::NUM_MATERIALS);

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

		std::vector<Material> materials;
		materials.resize(numMaterials * numMeshes);
		float tx{ initX };
		float ty{ initY };
		float tz{ initZ };
		for (std::size_t i = 0UL; i < numMaterials; ++i) {
			DirectX::XMFLOAT4X4 w;
			MathUtils::ComputeMatrix(w, tx, ty, tz, sS, sS, sS, DirectX::XM_PIDIV2);

			Material mat(Materials::GetMaterial(static_cast<Materials::MaterialType>(i)));
			for (std::size_t j = 0UL; j < numMeshes; ++j) {
				materials[i + j * numMaterials] = mat;
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
			materials.data(), 
			static_cast<std::uint32_t>(materials.size()));
	}
}

void ColorMappingScene::Init(ID3D12CommandQueue& cmdQueue) noexcept {
	Scene::Init(cmdQueue);

	// Load textures
	sResourceContainer.LoadTextures(sTexFiles, cmdQueue, *mCmdAlloc, *mCmdList, *mFence);

	// Load models
	sResourceContainer.LoadModels(sModelFiles, cmdQueue, *mCmdAlloc, *mCmdList, *mFence);
}

void ColorMappingScene::GenerateGeomPassRecorders(
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {

	ASSERT(tasks.empty());
	ASSERT(ValidateData());

	Model& model = sResourceContainer.GetModel(BUNNY);

	ColorCmdListRecorder* recorder{ nullptr };
	GenerateRecorder(sTx, sTy, sTz, sOffsetX, 0.0f, 0.0f, model.Meshes(), recorder);
	ASSERT(recorder != nullptr);
	tasks.push_back(std::unique_ptr<GeometryPassCmdListRecorder>(recorder));
}

void ColorMappingScene::GenerateLightingPassRecorders(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	std::vector<std::unique_ptr<LightingPassCmdListRecorder>>& tasks) noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	ASSERT(ValidateData());

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

void ColorMappingScene::GenerateCubeMaps(
	ID3D12Resource* &skyBoxCubeMap,
	ID3D12Resource* &diffuseIrradianceCubeMap,
	ID3D12Resource* &specularPreConvolvedCubeMap) noexcept 
{
	skyBoxCubeMap = &sResourceContainer.GetResource(SKY_BOX);
	diffuseIrradianceCubeMap = &sResourceContainer.GetResource(DIFFUSE_CUBE_MAP);
	specularPreConvolvedCubeMap = &sResourceContainer.GetResource(SPECULAR_CUBE_MAP);
}

