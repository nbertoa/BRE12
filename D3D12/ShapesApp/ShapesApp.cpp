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

	GeometryGenerator::MeshData sphere{ GeometryGenerator::CreateSphere(2, 100, 100) };
	GeometryGenerator::MeshData box{ GeometryGenerator::CreateBox(2, 2, 2, 10) };

	RenderTaskInitData initData{};
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

	for (std::size_t k = 0UL; k < 4; ++k) {
		initData.mMeshInfoVec.clear();
		const std::string taskName{ "task" + std::to_string(k) };
		ShapeTask* shapeTask{ new ShapeTask(taskName.c_str(), mD3dDevice.Get(), mScreenViewport, mScissorRect) };

		for (std::size_t i = 0UL; i < 250; ++i) {
			const float tx{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
			const float ty{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };
			const float tz{ MathHelper::RandF(-meshSpaceOffset, meshSpaceOffset) };

			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(tx, ty, tz));
			initData.mMeshInfoVec.push_back(MeshInfo(&box, world));
		}

		shapeTask->Init(initData, mCmdLists);
		mShapeTasks.push_back(shapeTask);
	}
	
	ID3D12CommandList* cmdLists[3]{ nullptr };
	std::uint32_t curNumLists = 0U;
	for (ID3D12CommandList* cmd : mCmdLists) {
		if (curNumLists == 3U) {
			mCmdQueue->ExecuteCommandLists(curNumLists, &cmdLists[0]);
			curNumLists = 0U;
		}
		cmdLists[curNumLists] = cmd;
		++curNumLists;
	}
	if (curNumLists > 0) {
		mCmdQueue->ExecuteCommandLists(curNumLists, &cmdLists[0]);
	}

	mCmdLists.clear();
	FlushCommandQueue();

	/*

	const float baseOffset{ 10.0f };
	for (std::size_t i = 0UL; i < numMeshes; ++i) {
	const MeshInfo& meshInfo{ initData.mMeshInfoVec[i] };
	ASSERT(meshInfo.mData != nullptr);
	const GeometryGenerator::MeshData& meshData{ *meshInfo.mData };
	const std::uint32_t numVertices = (std::uint32_t)meshData.mVertices.size();
	std::vector<Vertex> vertices;
	vertices.reserve(numVertices);
	for (const GeometryGenerator::Vertex& vtx : meshData.mVertices) {
	vertices.push_back(Vertex{ DirectX::XMFLOAT4(vtx.mPosition.x, vtx.mPosition.y, vtx.mPosition.z, 1.0f) });
	}

	GeometryData geomData;
	BuildVertexAndIndexBuffers(geomData, vertices.data(), numVertices, sizeof(Vertex), meshData.mIndices32.data(), (std::uint32_t)meshData.mIndices32.size());
	geomData.mWorld = meshInfo.mWorld;
	mGeomDataVec.push_back(geomData);
	}*/
}

void ShapesApp::Update(const float dt) noexcept {
	App::Update(dt);
	
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, mShapeTasks.size()), 
		[&](const tbb::blocked_range<size_t>& r) {		
			for (size_t i = r.begin(); i != r.end(); ++i)
				mShapeTasks[i]->Update();
			}
		);
}

void ShapesApp::Draw(const float) noexcept {
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
			mShapeTasks[i]->BuildCmdLists(mCmdLists, backBufferHandle, dsvHandle);
	}
	);

	ID3D12CommandList* execCmdLists[10]{ nullptr };
	std::uint32_t curNumLists = 0U;
	for (ID3D12CommandList* cmd : mCmdLists) {
		if (curNumLists == 10U) {
			mCmdQueue->ExecuteCommandLists(curNumLists, &execCmdLists[0]);
			curNumLists = 0U;
		}
		execCmdLists[curNumLists] = cmd;
		++curNumLists;
	}
	if (curNumLists > 0) {
		mCmdQueue->ExecuteCommandLists(curNumLists, &execCmdLists[0]);
	}
	mCmdLists.clear();

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