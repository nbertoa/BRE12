#include "ShapesApp.h"

#include <DirectXColors.h>

#include <Camera\Camera.h>
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

	CHECK_HR(mCmdList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	Camera::gCamera->SetPosition(0.0f, 0.0f, -20.0f);
	Camera::gCamera->UpdateViewMatrix();

	BuildRootSignature();
	BuildPSO();
	BuildVertexAndIndexBuffers();
	BuildConstantBuffers();	

	CHECK_HR(mCmdList->Close());
	ID3D12CommandList* cmdLists[] = { mCmdList.Get() };
	mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	FlushCommandQueue();
}

void ShapesApp::Update(const float dt) noexcept {
	App::Update(dt);

	UpdateConstantBuffers();
}

void ShapesApp::Draw(const float) noexcept {
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	CHECK_HR(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	CHECK_HR(mCmdList->Reset(mDirectCmdListAlloc.Get(), mPSO));

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

	mCmdList->SetDescriptorHeaps(1U, mCBVHeap.GetAddressOf());
	ASSERT(mRootSignature != nullptr);
	mCmdList->SetGraphicsRootSignature(mRootSignature);
	ASSERT(mCBVHeap.Get());	
	const CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGpuDescHandle{ mCBVHeap->GetGPUDescriptorHandleForHeapStart() };
	//mCmdList->SetGraphicsRootDescriptorTable(0U, cbvGpuDescHandle);
	mCmdList->SetGraphicsRootConstantBufferView(0U, mCBVsUploadBuffer->Resource()->GetGPUVirtualAddress());

	mCmdList->IASetVertexBuffers(0U, 1U, &mVertexBufferView);
	mCmdList->IASetIndexBuffer(&mIndexBufferView);
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCmdList->DrawIndexedInstanced(mNumIndices, 1U, 0U, 0U, 0U);
	
	// Indicate a state transition on the resource usage.
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCmdList->ResourceBarrier(1U, &resBarrier);

	// Done recording commands.
	CHECK_HR(mCmdList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdLists[] = { mCmdList.Get() };
	mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// swap the back and front buffers
	ASSERT(mSwapChain.Get());
	CHECK_HR(mSwapChain->Present(0U, 0U));
	mCurrBackBuffer = (mCurrBackBuffer + 1U) % sSwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

void ShapesApp::BuildPSO() noexcept {
	ASSERT(mRootSignature != nullptr);

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout  
	{
		{"POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT, 0U, 0U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U}
	};

	ShaderManager::gManager->AddInputLayout("position_input_layout", inputLayout);

	D3D12_SHADER_BYTECODE vertexShader;
	ShaderManager::gManager->LoadShaderFile("ShapesApp/VS.cso", vertexShader);

	D3D12_SHADER_BYTECODE pixelShader;
	ShaderManager::gManager->LoadShaderFile("ShapesApp/PS.cso", pixelShader);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = mDepthStencilFormat;
	psoDesc.InputLayout = { inputLayout.data(), (std::uint32_t)inputLayout.size() };
	psoDesc.NumRenderTargets = 1U;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.pRootSignature = mRootSignature;
	psoDesc.PS = pixelShader;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RTVFormats[0U] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = 1U;
	psoDesc.SampleDesc.Quality = 0U;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.VS = vertexShader;

	PSOManager::gManager->CreateGraphicsPSO("default", psoDesc, mPSO);
}

void ShapesApp::BuildVertexAndIndexBuffers() noexcept {
	// Initialization data
	const GeometryGenerator::MeshData meshData = GeometryGenerator::CreateSphere(10, 100, 100);
	const std::uint32_t numVertices = (std::uint32_t)meshData.mVertices.size();
	std::vector<Vertex> vertices;
	vertices.reserve(numVertices);
	for (const GeometryGenerator::Vertex& vtx : meshData.mVertices) {
		vertices.push_back(Vertex{ DirectX::XMFLOAT4( vtx.mPosition.x, vtx.mPosition.y, vtx.mPosition.z, 1.0f) });
	}

	// Buffers creation
	std::uint32_t byteSize = numVertices * sizeof(Vertex);
	ResourceManager::gManager->CreateDefaultBuffer("vertex_buffer", *mCmdList.Get(), vertices.data(), byteSize, mVertexBuffer, mUploadVertexBuffer);
	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = byteSize;
	mVertexBufferView.StrideInBytes = sizeof(Vertex);

	mNumIndices = (std::uint32_t)meshData.mIndices32.size();
	byteSize = mNumIndices * sizeof(std::uint32_t);
	ResourceManager::gManager->CreateDefaultBuffer("index_buffer", *mCmdList.Get(), meshData.mIndices32.data(), byteSize, mIndexBuffer, mUploadIndexBuffer);
	mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
	mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	mIndexBufferView.SizeInBytes = byteSize;
}

void ShapesApp::BuildConstantBuffers() noexcept {
	// Create constant buffers descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = 1U;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ASSERT(!mCBVHeap.Get());
	ASSERT(mD3dDevice.Get());
	CHECK_HR(mD3dDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(mCBVHeap.GetAddressOf())));
	ASSERT(mCBVHeap.Get());

	// Create constant buffer.
	const std::size_t elemSize = UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4));
	ResourceManager::gManager->CreateUploadBuffer("cbv", elemSize, 1U, mCBVsUploadBuffer);

	// Create constant buffer view
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = mCBVsUploadBuffer->Resource()->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (std::uint32_t)elemSize;

	mD3dDevice->CreateConstantBufferView(&cbvDesc, mCBVHeap->GetCPUDescriptorHandleForHeapStart());
}

void ShapesApp::BuildRootSignature() noexcept {
	// Build root parameter: 1 desc table with 1 CBV
	/*CD3DX12_DESCRIPTOR_RANGE cbvTable{};
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1U, 0U);*/
	CD3DX12_ROOT_PARAMETER slotRootParameter{};
	//slotRootParameter.InitAsDescriptorTable(1U, &cbvTable);
	slotRootParameter.InitAsConstantBufferView(0U, 0U, D3D12_SHADER_VISIBILITY_VERTEX);

	// Build root signature
	const D3D12_ROOT_SIGNATURE_FLAGS flags =
	    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{ 1U, &slotRootParameter, 0U, nullptr, flags };
	RootSignatureManager::gManager->CreateRootSignature("root_signature", rootSigDesc, mRootSignature);
}

void ShapesApp::UpdateConstantBuffers() noexcept {
	DirectX::XMFLOAT4X4 wvpMatrix{};
	DirectX::XMStoreFloat4x4(&wvpMatrix, DirectX::XMMatrixTranspose(Camera::gCamera->GetViewProj()));
	mCBVsUploadBuffer->CopyData(0U, &wvpMatrix, sizeof(wvpMatrix));
}