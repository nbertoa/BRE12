#include "BasicScene.h"

#include <algorithm>
#include <tbb/parallel_for.h>

#include <DXUtils/Material.h>
#include <DXUtils/MaterialFactory.h>
#include <DXUtils/PunctualLight.h>
#include <GeometryPass/Recorders/BasicCmdListRecorder.h>
#include <GlobalData/D3dData.h>
#include <LightPass/Recorders/PunctualLightCmdListRecorder.h>
#include <MathUtils\MathUtils.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <SkyBoxPass/SkyBoxCmdListRecorder.h>

namespace {
	const char* sCubeMapFile{ "textures/snow2_cube_map.dds" };

	const float sS{ 4.0f };

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
		light[0].mPosAndRange[3] = 100000.0f;
		light[0].mColorAndPower[0] = 1.0f;
		light[0].mColorAndPower[1] = 1.0f;
		light[0].mColorAndPower[2] = 1.0f;
		light[0].mColorAndPower[3] = 1000000.0f;

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
		ID3D12Resource& cubeMap,
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

			Material mat(MaterialFactory::GetMaterial((MaterialFactory::MaterialType)i));
			for (std::size_t j = 0UL; j < numMeshes; ++j) {
				materials[i + j * numMaterials] = mat;
				GeometryPassCmdListRecorder::GeometryData& geomData{ geomDataVec[j] };
				geomData.mWorldMatrices.push_back(w);
			}

			tx += offsetX;
			ty += offsetY;
			tz += offsetZ;
		}

		recorder->Init(geomDataVec.data(), (std::uint32_t)geomDataVec.size(), materials.data(), (std::uint32_t)materials.size(), cubeMap);
	}
}

void BasicScene::GenerateGeomPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	CmdListHelper& cmdListHelper,
	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) const noexcept {
	ASSERT(tasks.empty());

	Model* model1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer1;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer1;
	ModelManager::Get().LoadModel("models/bunny.obj", model1, cmdListHelper.CmdList(), uploadVertexBuffer1, uploadIndexBuffer1);
	ASSERT(model1 != nullptr);

	Model* model2;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer2;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer2;
	ModelManager::Get().CreateSphere(1.0f, 50, 50, model2, cmdListHelper.CmdList(), uploadVertexBuffer2, uploadIndexBuffer2);
	ASSERT(model2 != nullptr);

	// Cube map texture
	ID3D12Resource* cubeMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex;
	ResourceManager::Get().LoadTextureFromFile(sCubeMapFile, cubeMap, uploadBufferTex, cmdListHelper.CmdList());
	ASSERT(cubeMap != nullptr);

	cmdListHelper.CloseCmdList();
	cmdListHelper.ExecuteCmdList();

	tasks.resize(2);
	BasicCmdListRecorder* basicRecorder{ nullptr };
	GenerateRecorder(sSphereTx, sSphereTy, sSphereTz, sSphereOffsetX, 0.0f, 0.0f, cmdListQueue, model1->Meshes(), *cubeMap, basicRecorder);
	ASSERT(basicRecorder != nullptr);
	tasks[0].reset(basicRecorder);

	BasicCmdListRecorder* basicRecorder2{ nullptr };
	GenerateRecorder(sBunnyTx, sBunnyTy, sBunnyTz, sBunnyOffsetX, 0.0f, 0.0f, cmdListQueue, model2->Meshes(), *cubeMap, basicRecorder2);
	ASSERT(basicRecorder2 != nullptr);
	tasks[1].reset(basicRecorder2);
}

void BasicScene::GenerateLightPassRecorders(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	CmdListHelper& cmdListHelper,
	std::vector<std::unique_ptr<LightPassCmdListRecorder>>& tasks) const noexcept
{
	ASSERT(tasks.empty());
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	tasks.resize(1UL);
	PunctualLightCmdListRecorder* recorder{ nullptr };
	GenerateRecorder(cmdListQueue, geometryBuffers, geometryBuffersCount, recorder);
	ASSERT(recorder != nullptr);
	tasks[0].reset(recorder);

	cmdListHelper.CloseCmdList();
}

void BasicScene::GenerateSkyBoxRecorder(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	CmdListHelper& cmdListHelper,
	std::unique_ptr<SkyBoxCmdListRecorder>& task) const noexcept
{
	SkyBoxCmdListRecorder* recorder = new SkyBoxCmdListRecorder(D3dData::Device(), cmdListQueue);

	// Sky box sphere
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().CreateSphere(3000, 50, 50, model, cmdListHelper.CmdList(), uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);
	const std::vector<Mesh>& meshes(model->Meshes());
	ASSERT(meshes.size() == 1UL);

	// Cube map texture
	ID3D12Resource* cubeMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferTex;
	ResourceManager::Get().LoadTextureFromFile(sCubeMapFile, cubeMap, uploadBufferTex, cmdListHelper.CmdList());
	ASSERT(cubeMap != nullptr);

	cmdListHelper.CloseCmdList();
	cmdListHelper.ExecuteCmdList();

	// Build world matrix
	const Mesh& mesh{ meshes[0] };
	DirectX::XMFLOAT4X4 w;
	MathUtils::ComputeMatrix(w, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, DirectX::XM_PI, 0.0f);

	// Init recorder and store in task
	recorder->Init(mesh.VertexBufferData(), mesh.IndexBufferData(), w, *cubeMap);
	task.reset(recorder);
}

