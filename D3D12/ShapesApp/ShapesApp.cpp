#include "ShapesApp.h"

#include <App/App.h>
#include <DXutils/D3DFactory.h>
#include <GeometryGenerator\GeometryGenerator.h>
#include <PSOManager\PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShapesApp/ShapeInitTask.h>
#include <ShapesApp/ShapeTask.h>
#include <Utils\DebugUtils.h>

namespace {
	struct Vertex {
		Vertex() {}
		Vertex(const DirectX::XMFLOAT4& p)
			: mPosition(p)
		{}

		DirectX::XMFLOAT4 mPosition{ 0.0f, 0.0f, 0.0f, 0.0f };
	};
}

void ShapesApp::Run(App& app) noexcept {
	app.Initialize();

	GeometryGenerator::MeshData sphere{ GeometryGenerator::CreateSphere(2, 100, 100) };
	GeometryGenerator::MeshData box{ GeometryGenerator::CreateBox(2, 2, 2, 10) };

	const std::size_t numTasks{ 5UL };
	std::vector<std::unique_ptr<InitTask>>& initTasks(app.InitTasks());
	initTasks.resize(numTasks);
	std::vector<std::unique_ptr<CmdBuilderTask>>& cmdBuilderTasks(app.CmdBuilderTasks());
	cmdBuilderTasks.resize(numTasks);
		
	// Build vertices and indices data
	const std::uint32_t numSphereVertices = (std::uint32_t)sphere.mVertices.size();
	std::vector<Vertex> sphereVerts;
	sphereVerts.reserve(numSphereVertices);
	for (const GeometryGenerator::Vertex& vtx : sphere.mVertices) {
		sphereVerts.push_back(Vertex{ DirectX::XMFLOAT4(vtx.mPosition.x, vtx.mPosition.y, vtx.mPosition.z, 1.0f) });
	}
	const std::uint32_t numBoxVertices = (std::uint32_t)box.mVertices.size();
	std::vector<Vertex> boxVerts;
	boxVerts.reserve(numBoxVertices);
	for (const GeometryGenerator::Vertex& vtx : box.mVertices) {
		boxVerts.push_back(Vertex{ DirectX::XMFLOAT4(vtx.mPosition.x, vtx.mPosition.y, vtx.mPosition.z, 1.0f) });
	}

	InitTaskInput initData{};
	initData.mInputLayout = D3DFactory::PosInputLayout();
	initData.mPSFilename = "ShapesApp/PS.cso";
	initData.mVSFilename = "ShapesApp/VS.cso";

	const float meshSpaceOffset{ 50.0f };
	for (std::size_t k = 0UL; k < numTasks; ++k) {
		initTasks[k].reset(new ShapeInitTask());
		cmdBuilderTasks[k].reset(new ShapeTask(&app.Device(), app.Viewport(), app.ScissorRect()));

		initData.mMeshInfoVec.clear();
		for (std::size_t i = 0UL; i < 20; ++i) {
			const float tx{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
			const float ty{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
			const float tz{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };

			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
			initData.mMeshInfoVec.push_back(MeshInfo(boxVerts.data(), numBoxVertices, box.mIndices32.data(), (std::uint32_t)box.mIndices32.size(),  world));
		}

		initTasks[k]->TaskInput() = initData;
	}

	app.InitializeTasks();
	app.Run();
}