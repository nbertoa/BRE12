#include "TextureScene.h"

#include <tbb/parallel_for.h>

#include <GeometryPass/Recorders/TextureCmdListRecorder.h>
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

void TextureScene::GenerateGeomPassRecorders(
	ID3D12CommandQueue& cmdQueue,
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {
	ASSERT(tasks.empty());
	ASSERT(ValidateData());

	CHECK_HR(mCmdList->Reset(mCmdAlloc, nullptr));

	const std::size_t numGeometry{ 100UL };
	tasks.resize(Settings::sCpuProcessors);
	
	ID3D12Resource* tex[2] = { nullptr, nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex0;
	ResourceManager::Get().LoadTextureFromFile("textures/brick/brick.dds", tex[0], uploadBufferTex0, *mCmdList);
	ASSERT(tex[0] != nullptr);
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex1;
	ResourceManager::Get().LoadTextureFromFile("textures/brick/brick3.dds", tex[1], uploadBufferTex1, *mCmdList);
	ASSERT(tex[1] != nullptr);

	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().LoadModel("models/mitsubaSphere.obj", model, *mCmdList, uploadVertexBuffer, uploadIndexBuffer);
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

	const float meshSpaceOffset{ 100.0f };
	const float scaleFactor{ 0.2f };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, Settings::sCpuProcessors, numGeometry),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t k = r.begin(); k != r.end(); ++k) {
			TextureCmdListRecorder& task{ *new TextureCmdListRecorder(D3dData::Device()) };
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
				textures.push_back(tex[i % _countof(tex)]);
			}

			task.Init(&currGeomData, 1U, materials.data(), textures.data(), static_cast<std::uint32_t>(textures.size()));
		}
	}
	);
}

void TextureScene::GenerateLightingPassRecorders(
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

void TextureScene::GenerateCubeMaps(
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