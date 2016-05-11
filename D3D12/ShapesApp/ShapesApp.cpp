#include "ShapesApp.h"

#include <DirectXColors.h>

#include <Camera\Camera.h>
#include <DXUtils\D3dUtils.h>
#include <DXUtils\d3dx12.h>
#include <GeometryGenerator\GeometryGenerator.h>
#include <PSOManager\PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils\DebugUtils.h>

ShapesApp::ShapesApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

void ShapesApp::Initialize() noexcept {
	D3DApp::Initialize();

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

void ShapesApp::Update(const Timer& timer) noexcept {
	D3DApp::Update(timer);

	DirectX::XMFLOAT4X4 wvpMatrix{};
	DirectX::XMStoreFloat4x4(&wvpMatrix, DirectX::XMMatrixTranspose(Camera::gCamera->GetViewProj()));
	mCBVsUploadBuffer->CopyData(0U, &wvpMatrix, sizeof(wvpMatrix));
}

void ShapesApp::Draw(const Timer& /*timer*/) noexcept {
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	CHECK_HR(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	CHECK_HR(mCmdList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

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
	ASSERT(mRootSignature.Get());
	mCmdList->SetGraphicsRootSignature(mRootSignature.Get());
	ASSERT(mCBVHeap.Get());	
	const CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGpuDescHandle{ mCBVHeap->GetGPUDescriptorHandleForHeapStart() };
	mCmdList->SetGraphicsRootDescriptorTable(0U, cbvGpuDescHandle);

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
	ASSERT(mRootSignature.Get());

	D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = 
	{
		{"POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT, 0U, 0U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U}
	};

	D3D12_SHADER_BYTECODE vertexShader;
	std::string filename = "ShapesApp/VS.cso";
	ShaderManager::gShaderMgr->LoadShaderFile(filename, vertexShader);

	D3D12_SHADER_BYTECODE pixelShader;
	filename = "ShapesApp/PS.cso";
	ShaderManager::gShaderMgr->LoadShaderFile(filename, pixelShader);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = mDepthStencilFormat;
	psoDesc.InputLayout = { inputLayoutDesc, _countof(inputLayoutDesc) };
	psoDesc.NumRenderTargets = 1U;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.PS = pixelShader;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RTVFormats[0U] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = 1U;
	psoDesc.SampleDesc.Quality = 0U;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.VS = vertexShader;

	const std::string psoName = "default";
	PSOManager::gPSOMgr->CreateGraphicsPSO(psoName, psoDesc, mPSO);
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
	ResourceManager::gResourceMgr->CreateDefaultBuffer(*mD3dDevice.Get(), *mCmdList.Get(), vertices.data(), byteSize, mVertexBuffer, mUploadVertexBuffer);
	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = byteSize;
	mVertexBufferView.StrideInBytes = sizeof(Vertex);

	mNumIndices = (std::uint32_t)meshData.mIndices32.size();
	byteSize = mNumIndices * sizeof(std::uint32_t);
	ResourceManager::gResourceMgr->CreateDefaultBuffer(*mD3dDevice.Get(), *mCmdList.Get(), meshData.mIndices32.data(), byteSize, mIndexBuffer, mUploadIndexBuffer);
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
	ASSERT(!mCBVsUploadBuffer.get());
	const std::uint32_t elemSize = D3dUtils::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4));
	mCBVsUploadBuffer = std::make_unique<UploadBuffer>(*mD3dDevice.Get(), elemSize, 1U);

	// Create constant buffer view
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = mCBVsUploadBuffer->Resource()->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = elemSize;

	mD3dDevice->CreateConstantBufferView(&cbvDesc, mCBVHeap->GetCPUDescriptorHandleForHeapStart());
}

void ShapesApp::BuildRootSignature() noexcept {
	// Build root parameter
	CD3DX12_DESCRIPTOR_RANGE cbvTable{};
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1U, 0U);
	CD3DX12_ROOT_PARAMETER slotRootParameter{};
	slotRootParameter.InitAsDescriptorTable(1U, &cbvTable);

	// Build root signature
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1U, &slotRootParameter, 0U, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob{nullptr};
	CHECK_HR(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
	ASSERT(mD3dDevice.Get());
	CHECK_HR(mD3dDevice->CreateRootSignature(0U, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}