#include "InitDirect3DApp.h"

#include <DirectXColors.h>

#include <DXUtils\d3dx12.h>
#include <Utils\DebugUtils.h>

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

void InitDirect3DApp::Update(const Timer& /*timer*/) noexcept {

}

void InitDirect3DApp::Draw(const Timer& /*timer*/) noexcept {
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

	// Indicate a state transition on the resource usage.
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCmdList->ResourceBarrier(1U, &resBarrier);

	// Done recording commands.
	CHECK_HR(mCmdList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdLists[] = { mCmdList.Get() };
	mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// swap the back and front buffers
	CHECK_HR(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1U) % sSwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}