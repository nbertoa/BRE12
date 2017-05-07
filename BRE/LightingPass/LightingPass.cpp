#include "LightingPass.h"

#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DXUtils/d3dx12.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

using namespace DirectX;

void LightingPass::Init(
	ID3D12Resource& baseColorMetalMaskBuffer,
	ID3D12Resource& normalSmoothnessBuffer,
	ID3D12Resource& depthBuffer,
	ID3D12Resource& diffuseIrradianceCubeMap,
	ID3D12Resource& specularPreConvolvedCubeMap,
	const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept
{
	ASSERT(IsDataValid() == false);

	mBaseColorMetalMaskBuffer = &baseColorMetalMaskBuffer;
	mNormalSmoothnessBuffer = &normalSmoothnessBuffer;
	mRenderTargetView = renderTargetView;
	mDepthBuffer = &depthBuffer;
	
	// Initialize ambient pass
	mEnvironmentLightPass.Init(
		*mBaseColorMetalMaskBuffer,
		*mNormalSmoothnessBuffer,
		depthBuffer,
		diffuseIrradianceCubeMap,
		specularPreConvolvedCubeMap,
		renderTargetView);

	ASSERT(IsDataValid());
}

void LightingPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());

	ExecuteBeginTask();

	mEnvironmentLightPass.Execute(frameCBuffer);

	ExecuteFinalTask();
}

bool LightingPass::IsDataValid() const noexcept {

	const bool b =
		mBaseColorMetalMaskBuffer != nullptr &&
		mNormalSmoothnessBuffer != nullptr &&
		mRenderTargetView.ptr != 0UL &&
		mDepthBuffer != nullptr;

	return b;
}

void LightingPass::ExecuteBeginTask() noexcept {
	ASSERT(IsDataValid());

	// Check resource states:
	// - All geometry shaders must be in render target state because they were output
	// of the geometry pass.
#ifdef _DEBUG
	ASSERT(ResourceStateManager::GetResourceState(*mBaseColorMetalMaskBuffer) == D3D12_RESOURCE_STATE_RENDER_TARGET);
	ASSERT(ResourceStateManager::GetResourceState(*mNormalSmoothnessBuffer) == D3D12_RESOURCE_STATE_RENDER_TARGET);
#endif

	// - Depth buffer was used for depth testing in geometry pass, so it must be in depth write state
	ASSERT(ResourceStateManager::GetResourceState(*mDepthBuffer) == D3D12_RESOURCE_STATE_DEPTH_WRITE);

	ID3D12GraphicsCommandList& commandList = mBeginCommandListPerFrame.ResetWithNextCommandAllocator(nullptr);

	CD3DX12_RESOURCE_BARRIER barriers[]
	{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(
			*mBaseColorMetalMaskBuffer, 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

		ResourceStateManager::ChangeResourceStateAndGetBarrier(
			*mNormalSmoothnessBuffer, 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

		ResourceStateManager::ChangeResourceStateAndGetBarrier(
			*mDepthBuffer, 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};

	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == 3UL);
	commandList.ResourceBarrier(barriersCount, barriers);

	commandList.ClearRenderTargetView(mRenderTargetView, Colors::Black, 0U, nullptr);

	CHECK_HR(commandList.Close());
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}

void LightingPass::ExecuteFinalTask() noexcept {
	ASSERT(IsDataValid());

	// Check resource states:
	// - All geometry shaders must be in pixel shader resource state because they were used
	// by lighting pass shaders.
#ifdef _DEBUG
	ASSERT(ResourceStateManager::GetResourceState(*mBaseColorMetalMaskBuffer) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	ASSERT(ResourceStateManager::GetResourceState(*mNormalSmoothnessBuffer) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
#endif

	// - Depth buffer must be in pixel shader resource because it was used by lighting pass shader.
	ASSERT(ResourceStateManager::GetResourceState(*mDepthBuffer) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	ID3D12GraphicsCommandList& commandList = mFinalCommandListPerFrame.ResetWithNextCommandAllocator(nullptr);

	CD3DX12_RESOURCE_BARRIER barriers[]
	{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE),
	};

	const std::uint32_t barrierCount = _countof(barriers);
	ASSERT(barrierCount == 1UL);
	commandList.ResourceBarrier(barrierCount, barriers);

	CHECK_HR(commandList.Close());
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}