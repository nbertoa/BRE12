#include "ShapesApp.h"

#include <App/App.h>
#include <DXutils/D3DFactory.h>
#include <GeometryGenerator\GeometryGenerator.h>
#include <GlobalData/Settings.h>
#include <PSOManager\PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShapesApp/ShapeInitTask.h>
#include <ShapesApp/ShapeTask.h>
#include <Utils\DebugUtils.h>

void ShapesApp::Run(App& app) noexcept {
	app.Initialize();

	GeometryGenerator::MeshData sphere{ GeometryGenerator::CreateSphere(2, 100, 100) };
	GeometryGenerator::MeshData box{ GeometryGenerator::CreateBox(2, 2, 2, 2) };

	const std::size_t numTasks{ 15UL };
	const std::size_t numGeometry{ 100UL };
	std::vector<std::unique_ptr<InitTask>>& initTasks(app.InitTasks());
	initTasks.resize(numTasks);
	std::vector<std::unique_ptr<CmdBuilderTask>>& cmdBuilderTasks(app.CmdBuilderTasks());
	cmdBuilderTasks.resize(numTasks);

	InitTaskInput initData{};
	initData.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
	initData.mPSFilename = "ShapesApp/PS.cso";
	initData.mRootSignFilename = "ShapesApp/RS.cso";
	initData.mVSFilename = "ShapesApp/VS.cso";

	const float meshSpaceOffset{ 50.0f };
	for (std::size_t k = 0UL; k < numTasks; ++k) {
		initTasks[k].reset(new ShapeInitTask());
		cmdBuilderTasks[k].reset(new ShapeTask(&app.Device(), Settings::sScreenViewport, Settings::sScissorRect));

		initData.mMeshInfoVec.clear();
		for (std::size_t i = 0UL; i < numGeometry; ++i) {
			const float tx{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
			const float ty{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
			const float tz{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };

			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
			initData.mMeshInfoVec.push_back(MeshInfo(box.mVertices.data(), (std::uint32_t)box.mVertices.size(), box.mIndices32.data(), (std::uint32_t)box.mIndices32.size(),  world));
		}

		initTasks[k]->TaskInput() = initData;
	}

	app.InitializeTasks();
	app.Run();
}