#include "NormalScene.h"

#include <tbb/parallel_for.h>

#include <GeometryPass/Material.h>
#include <GeometryPass/Recorders/NormalCmdListRecorder.h>
#include <GlobalData/D3dData.h>
#include <LightPass/PunctualLight.h>
#include <LightPass/Recorders/PunctualLightCmdListRecorder.h>
#include <MathUtils/MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <SkyBoxPass/SkyBoxCmdListRecorder.h>

namespace {
	const char* sCubeMapFile{ "textures/snow2_cube_map.dds" };
}

void NormalScene::GenerateGeomPassRecorders(
	ID3D12CommandQueue& cmdQueue,
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {

	ASSERT(tasks.empty());
	ASSERT(ValidateData());

	CHECK_HR(mCmdList->Reset(mCmdAlloc, nullptr));

	const std::size_t numGeometry{ 100UL };
	tasks.resize(Settings::sCpuProcessors);

	ID3D12Resource* tex[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex[_countof(tex)];
	ResourceManager::Get().LoadTextureFromFile("textures/bricks.dds", tex[0], uploadBufferTex[0], *mCmdList);
	ASSERT(tex[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2.dds", tex[1], uploadBufferTex[1], *mCmdList);
	ASSERT(tex[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock.dds", tex[2], uploadBufferTex[2], *mCmdList);
	ASSERT(tex[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor.dds", tex[3], uploadBufferTex[3], *mCmdList);
	ASSERT(tex[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete.dds", tex[4], uploadBufferTex[4], *mCmdList);
	ASSERT(tex[4] != nullptr);

	ID3D12Resource* normal[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferNormal[_countof(normal)];
	ResourceManager::Get().LoadTextureFromFile("textures/bricks_normal.dds", normal[0], uploadBufferNormal[0], *mCmdList);
	ASSERT(normal[0] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock2_normal.dds", normal[1], uploadBufferNormal[1], *mCmdList);
	ASSERT(normal[1] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/rock_normal.dds", normal[2], uploadBufferNormal[2], *mCmdList);
	ASSERT(normal[2] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/floor_normal.dds", normal[3], uploadBufferNormal[3], *mCmdList);
	ASSERT(normal[3] != nullptr);
	ResourceManager::Get().LoadTextureFromFile("textures/concrete_normal.dds", normal[4], uploadBufferNormal[4], *mCmdList);
	ASSERT(normal[4] != nullptr);

	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().CreateSphere(4.0f, 50, 50, model, *mCmdList, uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);

	// Cube map texture
	ID3D12Resource* cubeMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferCubeMap;
	ResourceManager::Get().LoadTextureFromFile(sCubeMapFile, cubeMap, uploadBufferCubeMap, *mCmdList);
	ASSERT(cubeMap != nullptr);

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

	const float meshSpaceOffset{ 100.0f };
	const float scaleFactor{ 2.0f };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, Settings::sCpuProcessors, numGeometry),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			NormalCmdListRecorder& task{ *new NormalCmdListRecorder(D3dData::Device(), cmdListQueue) };
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
				material.mBaseColor_MetalMask[0] = 1.0f;
				material.mBaseColor_MetalMask[1] = 1.0f;
				material.mBaseColor_MetalMask[2] = 1.0f;
				material.mBaseColor_MetalMask[3] = static_cast<float>(MathUtils::Rand(0U, 1U));
				material.mSmoothness = MathUtils::RandF(0.0f, 1.0f);
				materials.push_back(material);
			}

			std::vector<ID3D12Resource*> textures;
			textures.reserve(numGeometry);
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				textures.push_back(tex[i % _countof(tex)]);
			}

			std::vector<ID3D12Resource*> normals;
			normals.reserve(numGeometry);
			for (std::size_t i = 0UL; i < numGeometry; ++i) {
				normals.push_back(normal[i % _countof(normal)]);
			}

			task.Init(
				&currGeomData, 
				1U, 
				materials.data(), 
				textures.data(), 
				normals.data(), 
				static_cast<std::uint32_t>(normals.size()),
				*cubeMap);
		}
	}
	);
}

void NormalScene::GenerateLightPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	std::vector<std::unique_ptr<LightPassCmdListRecorder>>& tasks) noexcept
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
			PunctualLightCmdListRecorder& task{ *new PunctualLightCmdListRecorder(D3dData::Device(), cmdListQueue) };
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

			task.Init(geometryBuffers, geometryBuffersCount, light, _countof(light));
		}
	}
	);
}

void NormalScene::GenerateSkyBoxRecorder(
	ID3D12CommandQueue& cmdQueue,
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	std::unique_ptr<SkyBoxCmdListRecorder>& task) noexcept
{
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
	ResourceManager::Get().LoadTextureFromFile(sCubeMapFile, cubeMap, uploadBufferTex, *mCmdList);
	ASSERT(cubeMap != nullptr);

	ExecuteCommandList(cmdQueue);

	// Build world matrix
	const Mesh& mesh{ meshes[0] };
	DirectX::XMFLOAT4X4 w;
	MathUtils::ComputeMatrix(w, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, DirectX::XM_PI, 0.0f);

	// Init recorder and store in task
	recorder->Init(mesh.VertexBufferData(), mesh.IndexBufferData(), w, *cubeMap);
	task.reset(recorder);
}

