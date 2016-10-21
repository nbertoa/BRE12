#include "BasicScene.h"

#include <tbb/parallel_for.h>

#include <GeometryPass/Material.h>
#include <GeometryPass/MaterialFactory.h>
#include <GeometryPass/Recorders/BasicCmdListRecorder.h>
#include <GlobalData/D3dData.h>
#include <LightPass/PunctualLight.h>
#include <LightPass/Recorders/PunctualLightCmdListRecorder.h>
#include <MathUtils\MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <SkyBoxPass/SkyBoxCmdListRecorder.h>

namespace {
	const char* sSkyBoxFile{ "textures/milkmill_cube_map.dds" };
	const char* sDiffuseEnvironmentFile{ "textures/milkmill_diffuse_cube_map.dds" };
	const char* sSpecularEnvironmentFile{ "textures/milkmill_specular_cube_map.dds" };

	const float sS{ 5.0f };

	const float sSphereTx{ 0.0f };
	const float sSphereTy{ -3.5f };
	const float sSphereTz{ 10.0f };	
	const float sSphereOffsetX{ 15.0f };

	const float sBunnyTx{ 0.0f };
	const float sBunnyTy{ -3.5f };
	const float sBunnyTz{ -5.0f };
	const float sBunnyOffsetX{ 15.0f };

	void GenerateRecorder(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		PunctualLightCmdListRecorder* &recorder) {
		recorder = new PunctualLightCmdListRecorder(D3dData::Device(), cmdListQueue);
		PunctualLight light[1];
		light[0].mPosAndRange[0] = 0.0f;
		light[0].mPosAndRange[1] = 300.0f;
		light[0].mPosAndRange[2] = -100.0f;
		light[0].mPosAndRange[3] = 5000.0f;
		light[0].mColorAndPower[0] = 1.0f;
		light[0].mColorAndPower[1] = 1.0f;
		light[0].mColorAndPower[2] = 1.0f;
		light[0].mColorAndPower[3] = 10000000.0f;

		recorder->Init(geometryBuffers, geometryBuffersCount, light, _countof(light));
	}

	void GenerateRecorder(
		const float initX,
		const float initY, 
		const float initZ,
		const float offsetX,
		const float offsetY,
		const float offsetZ,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		const std::vector<Mesh>& meshes,
		ID3D12Resource& diffuseCubeMap,
		ID3D12Resource& specularCubeMap,
		BasicCmdListRecorder* &recorder) {
		recorder = new BasicCmdListRecorder(D3dData::Device(), cmdListQueue);

		const std::size_t numMaterials(MaterialFactory::NUM_MATERIALS);

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
			MathUtils::ComputeMatrix(w, tx, ty, tz, sS, sS, sS);

			Material mat(MaterialFactory::GetMaterial(static_cast<MaterialFactory::MaterialType>(i)));
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
			static_cast<std::uint32_t>(materials.size()), 
			diffuseCubeMap,
			specularCubeMap);
	}
}

void BasicScene::GenerateGeomPassRecorders(
	ID3D12CommandQueue& cmdQueue,
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept {

	ASSERT(tasks.empty());
	ASSERT(ValidateData());

	CHECK_HR(mCmdList->Reset(mCmdAlloc, nullptr));

	Model* model1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer1;
	ModelManager::Get().LoadModel("models/bunny.obj", model1, *mCmdList, uploadVertexBuffer1, uploadIndexBuffer1);
	ASSERT(model1 != nullptr);

	Model* model2;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer2;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer2;
	ModelManager::Get().CreateSphere(1.0f, 50, 50, model2, *mCmdList, uploadVertexBuffer2, uploadIndexBuffer2);
	ASSERT(model2 != nullptr);

	// Cube map textures
	ID3D12Resource* diffuseCubeMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex;
	ResourceManager::Get().LoadTextureFromFile(sDiffuseEnvironmentFile, diffuseCubeMap, uploadBufferTex, *mCmdList);
	ASSERT(diffuseCubeMap != nullptr);

	ID3D12Resource* specularCubeMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex2;
	ResourceManager::Get().LoadTextureFromFile(sSpecularEnvironmentFile, specularCubeMap, uploadBufferTex2, *mCmdList);
	ASSERT(specularCubeMap != nullptr);

	ExecuteCommandList(cmdQueue);

	tasks.resize(2);
	BasicCmdListRecorder* basicRecorder{ nullptr };
	GenerateRecorder(sSphereTx, sSphereTy, sSphereTz, sSphereOffsetX, 0.0f, 0.0f, cmdListQueue, model1->Meshes(), *diffuseCubeMap, *specularCubeMap, basicRecorder);
	ASSERT(basicRecorder != nullptr);
	tasks[0].reset(basicRecorder);

	BasicCmdListRecorder* basicRecorder2{ nullptr };
	GenerateRecorder(sBunnyTx, sBunnyTy, sBunnyTz, sBunnyOffsetX, 0.0f, 0.0f, cmdListQueue, model2->Meshes(), *diffuseCubeMap, *specularCubeMap, basicRecorder2);
	ASSERT(basicRecorder2 != nullptr);
	tasks[1].reset(basicRecorder2);
}

void BasicScene::GenerateLightPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	std::vector<std::unique_ptr<LightPassCmdListRecorder>>& tasks) noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	ASSERT(ValidateData());

	tasks.resize(1UL);
	PunctualLightCmdListRecorder* recorder{ nullptr };
	GenerateRecorder(cmdListQueue, geometryBuffers, geometryBuffersCount, recorder);
	ASSERT(recorder != nullptr);
	tasks[0].reset(recorder);
}

void BasicScene::GenerateSkyBoxRecorder(
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
	ResourceManager::Get().LoadTextureFromFile(sSkyBoxFile, cubeMap, uploadBufferTex, *mCmdList);
	ASSERT(cubeMap != nullptr);

	ExecuteCommandList(cmdQueue);

	// Build world matrix
	const Mesh& mesh{ meshes[0] };
	DirectX::XMFLOAT4X4 w;
	MathUtils::ComputeMatrix(w, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// Init recorder and store in task
	recorder->Init(mesh.VertexBufferData(), mesh.IndexBufferData(), w, *cubeMap);
	task.reset(recorder);
}

