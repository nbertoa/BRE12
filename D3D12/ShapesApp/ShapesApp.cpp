#include "ShapesApp.h"

#include <DXutils/D3DFactory.h>
#include <GeometryGenerator\GeometryGenerator.h>
#include <GlobalData/D3dData.h>
#include <ShapesApp/ShapesInitTask.h>
#include <ShapesApp/ShapesCmdBuilderTask.h>
#include <Utils\DebugUtils.h>

void ShapesApp::InitTasks(App& app) noexcept {
	GeometryGenerator::MeshData sphere{ GeometryGenerator::CreateSphere(2, 100, 100) };
	GeometryGenerator::MeshData box{ GeometryGenerator::CreateBox(2, 2, 2, 2) };

	const std::size_t numTasks{ 15UL };
	const std::size_t numGeometry{ 100UL };
	std::vector<std::unique_ptr<InitTask>>& initTasks(app.GetInitTasks());
	initTasks.resize(numTasks);
	std::vector<std::unique_ptr<CmdBuilderTask>>& cmdBuilderTasks(app.GetCmdBuilderTasks());
	cmdBuilderTasks.resize(numTasks);

	InitTaskInput initData{};
	initData.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
	initData.mPSFilename = "ShapesApp/PS.cso";
	initData.mRootSignFilename = "ShapesApp/RS.cso";
	initData.mVSFilename = "ShapesApp/VS.cso";

	const float meshSpaceOffset{ 50.0f };
	for (std::size_t k = 0UL; k < numTasks; ++k) {
		initTasks[k].reset(new ShapesInitTask());
		cmdBuilderTasks[k].reset(new ShapesCmdBuilderTask(D3dData::mDevice.Get(), Settings::sScreenViewport, Settings::sScissorRect));

		initData.mMeshInfoVec.clear();
		for (std::size_t i = 0UL; i < numGeometry; ++i) {
			const float tx{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
			const float ty{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
			const float tz{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };

			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
			initData.mMeshInfoVec.push_back(GeometryInfo(box.mVertices.data(), (std::uint32_t)box.mVertices.size(), box.mIndices32.data(), (std::uint32_t)box.mIndices32.size(), world));
		}

		initTasks[k]->TaskInput() = initData;
	}

	app.InitCmdBuilders();
}