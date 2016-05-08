#include "ShapesApp.h"

#include <DirectXColors.h>

#include <DXUtils\d3dx12.h>
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

	BuildPSO();
}

void ShapesApp::Update(const Timer& /*timer*/) noexcept {

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

	//
	// Draw
	//

	// Indicate a state transition on the resource usage.
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCmdList->ResourceBarrier(1U, &resBarrier);

	// Done recording commands.
	CHECK_HR(mCmdList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdLists[] = { mCmdList.Get() };
	mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// swap the back and front buffers
	CHECK_HR(mSwapChain->Present(0U, 0U));
	mCurrBackBuffer = (mCurrBackBuffer + 1U) % sSwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

void ShapesApp::BuildPSO() noexcept {
	D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = 
	{
		{"POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT, 0U, 0U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U},
		{ "COLOR", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_APPEND_ALIGNED_ELEMENT, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U}
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
	psoDesc.InputLayout = { inputLayoutDesc, _ARRAYSIZE(inputLayoutDesc) };
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