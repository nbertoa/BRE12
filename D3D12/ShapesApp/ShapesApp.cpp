#include "ShapesApp.h"

#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <Camera\Camera.h>
#include <DXutils/D3DFactory.h>
#include <DXUtils\d3dx12.h>
#include <GeometryGenerator\GeometryGenerator.h>
#include <PSOManager\PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils\DebugUtils.h>

ShapesApp::ShapesApp(HINSTANCE hInstance)
	: App(hInstance)
{
}

void ShapesApp::Initialize() noexcept {
	App::Initialize();

	Camera::gCamera->UpdateViewMatrix();

	GeometryGenerator::MeshData sphere{ GeometryGenerator::CreateSphere(2, 100, 100) };
	GeometryGenerator::MeshData box{ GeometryGenerator::CreateBox(2, 2, 2, 10) };

	const std::size_t numInitTasks{ 200UL };

	std::vector<ShapeInitTask*> initTasks{ numInitTasks };
	for (std::size_t i = 0UL; i < numInitTasks; ++i) {
		initTasks[i] = new ShapeInitTask();
	}
	
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

	CD3DX12_DESCRIPTOR_RANGE cbvTable{};
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1U, 0U);
	CD3DX12_ROOT_PARAMETER slotRootParameter{};
	slotRootParameter.InitAsDescriptorTable(1U, &cbvTable, D3D12_SHADER_VISIBILITY_VERTEX);

	// Build root signature
	const D3D12_ROOT_SIGNATURE_FLAGS flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	initData.mRootSignDesc = D3D12_ROOT_SIGNATURE_DESC{ 1U, &slotRootParameter, 0U, nullptr, flags };

	const float meshSpaceOffset{ 50.0f };

	std::vector<InitTaskInput> initDataVec;

	for (std::size_t k = 0UL; k < numInitTasks; ++k) {
		initData.mMeshInfoVec.clear();
		for (std::size_t i = 0UL; i < 10; ++i) {
			const float tx{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
			const float ty{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
			const float tz{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };

			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
			initData.mMeshInfoVec.push_back(MeshInfo(boxVerts.data(), numBoxVertices, box.mIndices32.data(), (std::uint32_t)box.mIndices32.size(),  world));
		}

		initDataVec.push_back(initData);
	}

	mCmdBuilderTaskInputs.resize(initTasks.size());

	mShapeTasks.resize(initTasks.size());
	for (std::size_t i = 0UL; i < mCmdBuilderTaskInputs.size(); ++i) {
		mShapeTasks[i] = new ShapeTask(mD3dDevice.Get(), mScreenViewport, mScissorRect, mCmdBuilderTaskInputs[i]);
	}

	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue{ mCmdListProcessor->CmdListQueue() };

	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, initTasks.size()),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			initTasks[i]->Execute(*mD3dDevice.Get(), initDataVec[i], cmdListQueue, mCmdBuilderTaskInputs[i]);
	}
	);

	FlushCommandQueue();
}

void ShapesApp::Update(const float dt) noexcept {
	App::Update(dt);
}

void ShapesApp::Draw(const float) noexcept {
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue{ mCmdListProcessor->CmdListQueue() };

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	CHECK_HR(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	CHECK_HR(mCmdList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	mCmdList->RSSetViewports(1U, &mScreenViewport);
	mCmdList->RSSetScissorRects(1U, &mScissorRect);

	// Indicate a state transition on the resource usage.
	CD3DX12_RESOURCE_BARRIER resBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET) };
	mCmdList->ResourceBarrier(1, &resBarrier);

	// Specify the buffers we are going to render to.
	const D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle = CurrentBackBufferView();
	const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();
	mCmdList->OMSetRenderTargets(1U, &backBufferHandle, true, &dsvHandle);

	// Clear the back buffer and depth buffer.
	mCmdList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0U, nullptr);
	mCmdList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0U, 0U, nullptr);

	// Done recording commands.
	CHECK_HR(mCmdList->Close());

	{
		ID3D12CommandList* cmdLists[] = { mCmdList.Get() };
		mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}

	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, mShapeTasks.size(), 20),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mShapeTasks[i]->Execute(cmdListQueue, backBufferHandle, dsvHandle);
	}
	);

	// Wait until all command lists are executed
	while (!cmdListQueue.empty()) {
	}

	CHECK_HR(mSecDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	CHECK_HR(mCmdList->Reset(mSecDirectCmdListAlloc.Get(), nullptr));
	
	// Indicate a state transition on the resource usage.
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCmdList->ResourceBarrier(1U, &resBarrier);

	// Done recording commands.
	CHECK_HR(mCmdList->Close());

	// Add the command list to the queue for execution.
	{
		ID3D12CommandList* cmdLists[] = { mCmdList.Get() };
		mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}

	// swap the back and front buffers
	ASSERT(mSwapChain.Get());
	CHECK_HR(mSwapChain->Present(0U, 0U));
	mCurrBackBuffer = (mCurrBackBuffer + 1U) % sSwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}