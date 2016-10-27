#include "BasicScene.h"

#include <tbb/parallel_for.h>

#include <GeometryPass/Recorders/BasicCmdListRecorder.h>
#include <GlobalData/D3dData.h>
#include <LightPass/PunctualLight.h>
#include <LightPass/Recorders/PunctualLightCmdListRecorder.h>
#include <Material/Material.h>
#include <Material/MaterialFactory.h>
#include <MathUtils\MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>

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
		ID3D12Resource& depthBuffer,
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

		recorder->Init(geometryBuffers, geometryBuffersCount, depthBuffer, light, _countof(light));
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
			static_cast<std::uint32_t>(materials.size()));
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

	ExecuteCommandList(cmdQueue);

	tasks.resize(2);
	BasicCmdListRecorder* basicRecorder{ nullptr };
	GenerateRecorder(sSphereTx, sSphereTy, sSphereTz, sSphereOffsetX, 0.0f, 0.0f, cmdListQueue, model1->Meshes(), basicRecorder);
	ASSERT(basicRecorder != nullptr);
	tasks[0].reset(basicRecorder);

	BasicCmdListRecorder* basicRecorder2{ nullptr };
	GenerateRecorder(sBunnyTx, sBunnyTy, sBunnyTz, sBunnyOffsetX, 0.0f, 0.0f, cmdListQueue, model2->Meshes(), basicRecorder2);
	ASSERT(basicRecorder2 != nullptr);
	tasks[1].reset(basicRecorder2);
}

void BasicScene::GenerateLightPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	std::vector<std::unique_ptr<LightPassCmdListRecorder>>& tasks) noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	ASSERT(ValidateData());

	tasks.resize(1UL);
	PunctualLightCmdListRecorder* recorder{ nullptr };
	GenerateRecorder(cmdListQueue, geometryBuffers, geometryBuffersCount, depthBuffer, recorder);
	ASSERT(recorder != nullptr);
	tasks[0].reset(recorder);
}

void BasicScene::GenerateCubeMaps(
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

