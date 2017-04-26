#include "TextureScene.h"

#include <tbb/parallel_for.h>

#include <DirectXManager\DirectXManager.h>
#include <GeometryPass/Recorders/TextureCmdListRecorder.h>
#include <LightingPass/PunctualLight.h>
#include <LightingPass/Recorders/PunctualLightCmdListRecorder.h>
#include <MaterialManager/Material.h>
#include <MathUtils/MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Scene/SceneUtils.h>

namespace {
	SceneUtils::SceneResources sResourceContainer;

	enum Textures {		
		BRICK,
		BRICK3,

		// Environment
		SKY_BOX,
		DIFFUSE_CUBE_MAP,
		SPECULAR_CUBE_MAP,

		TEXTURES_COUNT
	};

	// Textures to load
	std::vector<std::string> sTexFiles =
	{
		"resources/textures/brick/brick.dds",
		"resources/textures/brick/brick3.dds",

		// Environment
		"resources/textures/cubeMaps/milkmill_cube_map.dds",
		"resources/textures/cubeMaps/milkmill_diffuse_cube_map.dds",
		"resources/textures/cubeMaps/milkmill_specular_cube_map.dds",
	};

	enum Models {
		MITSUBA,
		MODELS_COUNT
	};

	// Models to load
	std::vector<std::string> sModelFiles =
	{
		"resources/models/unreal.obj",
	};
}

void TextureScene::Init() noexcept {
	Scene::Init();

	// Load textures
	sResourceContainer.LoadTextures(sTexFiles, *mCommandAllocator, *mCommandList);

	// Load models
	sResourceContainer.LoadModels(sModelFiles, *mCommandAllocator, *mCommandList);
}

void TextureScene::CreateGeometryPassRecorders(
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {
	ASSERT(tasks.empty());
	ASSERT(IsDataValid());

	const std::vector<ID3D12Resource*>& textures = sResourceContainer.GetTextures();
	ASSERT(textures.empty() == false);

	const Model& model = sResourceContainer.GetModel(MITSUBA);

	const std::size_t numGeometry{ 100UL };
	tasks.resize(SettingsManager::sCpuProcessorCount);

	ASSERT(model.HasMeshes());	
	const Mesh& mesh{ model.GetMeshes()[0U] };

	std::vector<GeometryPassCmdListRecorder::GeometryData> geomDataVec;
	geomDataVec.resize(SettingsManager::sCpuProcessorCount);
	for (GeometryPassCmdListRecorder::GeometryData& geomData : geomDataVec) {
		geomData.mVertexBufferData = mesh.GetVertexBufferData();
		geomData.mIndexBufferData = mesh.GetIndexBufferData();
		geomData.mWorldMatrices.reserve(numGeometry);
	}

	const float meshSpaceOffset{ 100.0f };
	const float scaleFactor{ 0.02f };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, SettingsManager::sCpuProcessorCount, numGeometry),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			TextureCmdListRecorder& task{ *new TextureCmdListRecorder() };
			tasks[k].reset(&task);
							
			GeometryPassCmdListRecorder::GeometryData& currGeomData{ geomDataVec[k] };
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				const float tx{ MathUtils::RandomFloatInInverval(-meshSpaceOffset, meshSpaceOffset) };
				const float ty{ MathUtils::RandomFloatInInverval(-meshSpaceOffset, meshSpaceOffset) };
				const float tz{ MathUtils::RandomFloatInInverval(-meshSpaceOffset, meshSpaceOffset) };

				const float s{ scaleFactor };

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixScaling(s, s, s) * DirectX::XMMatrixTranslation(tx, ty, tz));
				currGeomData.mWorldMatrices.push_back(world);
			}

			std::vector<Material> materials;
			materials.reserve(numGeometry);
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				Material material;
				material.RandomizeMaterial();
				materials.push_back(material);
			}

			std::vector<ID3D12Resource*> tex;
			tex.reserve(numGeometry);
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				tex.push_back(textures[i % 2]);
			}

			task.Init(&currGeomData, 1U, materials.data(), tex.data(), static_cast<std::uint32_t>(tex.size()));
		}
	}
	);
}

void TextureScene::CreateLightingPassRecorders(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	std::vector<std::unique_ptr<LightingPassCmdListRecorder>>& tasks) noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	ASSERT(IsDataValid());

	const std::uint32_t numTasks{ 1U };
	tasks.resize(numTasks);

	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, numTasks, 1U),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			PunctualLightCmdListRecorder& task{ *new PunctualLightCmdListRecorder() };
			tasks[k].reset(&task);
			PunctualLight light[2];
			light[0].mPositionAndRange[0] = 0.0f;
			light[0].mPositionAndRange[1] = 300.0f;
			light[0].mPositionAndRange[2] = 0.0f;
			light[0].mPositionAndRange[3] = 100000.0f;
			light[0].mColorAndPower[0] = 1.0f;
			light[0].mColorAndPower[1] = 1.0f;
			light[0].mColorAndPower[2] = 1.0f;
			light[0].mColorAndPower[3] = 1000000.0f;

			light[1].mPositionAndRange[0] = 0.0f;
			light[1].mPositionAndRange[1] = -300.0f;
			light[1].mPositionAndRange[2] = 0.0f;
			light[1].mPositionAndRange[3] = 100000.0f;
			light[1].mColorAndPower[0] = 1.0f;
			light[1].mColorAndPower[1] = 1.0f;
			light[1].mColorAndPower[2] = 1.0f;
			light[1].mColorAndPower[3] = 1000000.0f;

			task.Init(
				geometryBuffers, 
				geometryBuffersCount, 
				depthBuffer,
				light, 
				_countof(light));
		}		
	}
	);
}

void TextureScene::CreateIndirectLightingResources(
	ID3D12Resource* &skyBoxCubeMap,
	ID3D12Resource* &diffuseIrradianceCubeMap,
	ID3D12Resource* &specularPreConvolvedCubeMap) noexcept
{
	skyBoxCubeMap = &sResourceContainer.GetTexture(SKY_BOX);
	diffuseIrradianceCubeMap = &sResourceContainer.GetTexture(DIFFUSE_CUBE_MAP);
	specularPreConvolvedCubeMap = &sResourceContainer.GetTexture(SPECULAR_CUBE_MAP);
}