#include "HeightScene.h"

#include <tbb/parallel_for.h>

#include <GeometryPass/Recorders/HeightCmdListRecorder.h>
#include <GlobalData/D3dData.h>
#include <LightingPass/PunctualLight.h>
#include <LightingPass/Recorders/PunctualLightCmdListRecorder.h>
#include <Material/Material.h>
#include <MathUtils/MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>

namespace {
	const char* sSkyBoxFile{ "textures/cubeMaps/milkmill_cube_map.dds" };
	const char* sDiffuseEnvironmentFile{ "textures/cubeMaps/milkmill_diffuse_cube_map.dds" };
	const char* sSpecularEnvironmentFile{ "textures/cubeMaps/milkmill_specular_cube_map.dds" };
}

void HeightScene::GenerateGeomPassRecorders(
	ID3D12CommandQueue& cmdQueue,
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {

	ASSERT(tasks.empty());
	ASSERT(ValidateData());

	CHECK_HR(mCmdList->Reset(mCmdAlloc, nullptr));

	const std::size_t numGeometry{ 100UL };
	tasks.resize(Settings::sCpuProcessors);

	const std::uint32_t numResources{ 7U };

	ID3D12Resource* tex[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/black.dds", tex[0], uploadBufferTex[0], *mCmdList);
	ASSERT(tex[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/black.dds", tex[1], uploadBufferTex[1], *mCmdList);
	ASSERT(tex[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/black.dds", tex[2], uploadBufferTex[2], *mCmdList);
	ASSERT(tex[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/black.dds", tex[3], uploadBufferTex[3], *mCmdList);
	ASSERT(tex[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/black.dds", tex[4], uploadBufferTex[4], *mCmdList);
	ASSERT(tex[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/black.dds", tex[5], uploadBufferTex[5], *mCmdList);
	ASSERT(tex[5] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/black.dds", tex[6], uploadBufferTex[6], *mCmdList);
	ASSERT(tex[6] != nullptr);

	ID3D12Resource* normal[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferNormal[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock/rock_normal.dds", normal[0], uploadBufferNormal[0], *mCmdList);
	ASSERT(normal[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/brick/bricks_normal.dds", normal[1], uploadBufferNormal[1], *mCmdList);
	ASSERT(normal[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete/concrete_normal.dds", normal[2], uploadBufferNormal[2], *mCmdList);
	ASSERT(normal[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_normal.dds", normal[3], uploadBufferNormal[3], *mCmdList);
	ASSERT(normal[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/brick/bricks_normal.dds", normal[4], uploadBufferNormal[4], *mCmdList);
	ASSERT(normal[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock/rock2_normal.dds", normal[5], uploadBufferNormal[5], *mCmdList);
	ASSERT(normal[5] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/sand/sand_normal.dds", normal[6], uploadBufferNormal[6], *mCmdList);
	ASSERT(normal[6] != nullptr);

	ID3D12Resource* height[numResources];
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferHeight[numResources];
	ResourceManager::Get().LoadTextureFromFile("textures/rock/rock_height.dds", height[0], uploadBufferHeight[0], *mCmdList);
	ASSERT(height[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/brick/bricks_height.dds", height[1], uploadBufferHeight[1], *mCmdList);
	ASSERT(height[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete/concrete_height.dds", height[2], uploadBufferHeight[2], *mCmdList);
	ASSERT(height[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_height.dds", height[3], uploadBufferHeight[3], *mCmdList);
	ASSERT(height[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/brick/bricks_height.dds", height[4], uploadBufferHeight[4], *mCmdList);
	ASSERT(height[4] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock/rock2_height.dds", height[5], uploadBufferHeight[5], *mCmdList);
	ASSERT(height[5] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/sand/sand_height.dds", height[6], uploadBufferHeight[6], *mCmdList);
	ASSERT(height[6] != nullptr);
	
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().LoadModel("models/mitsubaFloor.obj", model, *mCmdList, uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);

	ExecuteCommandList(cmdQueue);

	ASSERT(model->HasMeshes());
	const Mesh& mesh{ model->Meshes()[0U] };

	std::vector<GeometryPassCmdListRecorder::GeometryData> geomDataVec;
	geomDataVec.resize(Settings::sCpuProcessors);
	for (GeometryPassCmdListRecorder::GeometryData& geomData : geomDataVec) {
		geomData.mVertexBufferData = mesh.VertexBufferData();
		geomData.mIndexBufferData = mesh.IndexBufferData();
		geomData.mWorldMatrices.reserve(numGeometry);
	}

	const float meshSpaceOffset{ 150.0f };
	const float scaleFactor{ 3.0f };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, Settings::sCpuProcessors, numGeometry),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			HeightCmdListRecorder& task{ *new HeightCmdListRecorder(D3dData::Device()) };
			tasks[k].reset(&task);

			GeometryPassCmdListRecorder::GeometryData& currGeomData{ geomDataVec[k] };
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
				material.RandomMaterial();
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

			task.Init(&currGeomData, 1U, materials.data(), textures.data(), normals.data(), heights.data(), static_cast<std::uint32_t>(normals.size()));
		}
	}
	);
}

void HeightScene::GenerateLightingPassRecorders(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	std::vector<std::unique_ptr<LightingPassCmdListRecorder>>& tasks) noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	ASSERT(ValidateData());

	const std::uint32_t numTasks{ 1U };
	tasks.resize(numTasks);

	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, numTasks, 1U),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			PunctualLightCmdListRecorder& task{ *new PunctualLightCmdListRecorder(D3dData::Device()) };
			tasks[k].reset(&task);
			PunctualLight light[2];
			light[0].mPosAndRange[0] = 0.0f;
			light[0].mPosAndRange[1] = 300.0f;
			light[0].mPosAndRange[2] = 0.0f;
			light[0].mPosAndRange[3] = 100000.0f;
			light[0].mColorAndPower[0] = 1.0f;
			light[0].mColorAndPower[1] = 1.0f;
			light[0].mColorAndPower[2] = 1.0f;
			light[0].mColorAndPower[3] = 1000000.0f;

			light[1].mPosAndRange[0] = 0.0f;
			light[1].mPosAndRange[1] = -300.0f;
			light[1].mPosAndRange[2] = 0.0f;
			light[1].mPosAndRange[3] = 100000.0f;
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

void HeightScene::GenerateCubeMaps(
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

