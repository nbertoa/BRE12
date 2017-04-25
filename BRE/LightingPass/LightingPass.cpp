#include "LightingPass.h"

#include <d3d12.h>
#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DXUtils/d3dx12.h>
#include <GeometryPass\GeometryPass.h>
#include <LightingPass\Recorders\PunctualLightCmdListRecorder.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

void LightingPass::Init(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	ID3D12Resource& diffuseIrradianceCubeMap,
	ID3D12Resource& specularPreConvolvedCubeMap,
	const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept
{
	ASSERT(IsDataValid() == false);

	mGeometryBuffers = geometryBuffers;
	mRenderTargetView = renderTargetView;
	mDepthBuffer = &depthBuffer;

	// Initialize recorder's pso
	PunctualLightCmdListRecorder::InitSharedPSOAndRootSignature();

	// Initialize ambient pass
	ASSERT(geometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get() != nullptr);
	ASSERT(geometryBuffers[GeometryPass::NORMAL_SMOOTHNESS].Get() != nullptr);
	mAmbientLightPass.Init(
		*geometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get(),
		*geometryBuffers[GeometryPass::NORMAL_SMOOTHNESS].Get(),		
		depthBuffer,
		renderTargetView);

	// Initialize environment light pass
	mEnvironmentLightPass.Init(
		geometryBuffers, 
		geometryBuffersCount,
		*mDepthBuffer,		
		diffuseIrradianceCubeMap,
		specularPreConvolvedCubeMap,
		renderTargetView);

	for (CommandListRecorders::value_type& recorder : mCommandListRecorders) {
		ASSERT(recorder.get() != nullptr);
		recorder->SetRenderTargetView(renderTargetView);
	}

	ASSERT(IsDataValid());
}

void LightingPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(IsDataValid());

	ExecuteBeginTask();

	CommandListExecutor::Get().ResetExecutedCommandListCount();
	const std::uint32_t lightTaskCount{ static_cast<std::uint32_t>(mCommandListRecorders.size())};
	
	// Execute tasks
	const std::uint32_t grainSize(max(1U, lightTaskCount / SettingsManager::sCpuProcessorCount));
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, lightTaskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mCommandListRecorders[i]->RecordAndPushCommandLists(frameCBuffer);
	}
	);
	
	// Wait until all previous tasks command lists are executed
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < lightTaskCount) {
		Sleep(0U);
	}

	mAmbientLightPass.Execute(frameCBuffer);
	mEnvironmentLightPass.Execute(frameCBuffer);

	ExecuteFinalTask();
}

bool LightingPass::IsDataValid() const noexcept {
	if (mGeometryBuffers == nullptr) {
		return false;
	}

	for (std::uint32_t i = 0U; i < GeometryPass::BUFFERS_COUNT; ++i) {
		if (mGeometryBuffers[i].Get() == nullptr) {
			return false;
		}
	}

	const bool b =
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
	for (std::uint32_t i = 0U; i < GeometryPass::BUFFERS_COUNT; ++i) {
		ASSERT(ResourceStateManager::GetResourceState(*mGeometryBuffers[i].Get()) == D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
#endif

	// - Depth buffer was used for depth testing in geometry pass, so it must be in depth write state
	ASSERT(ResourceStateManager::GetResourceState(*mDepthBuffer) == D3D12_RESOURCE_STATE_DEPTH_WRITE);

	ID3D12GraphicsCommandList& commandList = mBeginCommandListPerFrame.ResetWithNextCommandAllocator(nullptr);

	CD3DX12_RESOURCE_BARRIER barriers[]
	{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(
			*mGeometryBuffers[GeometryPass::NORMAL_SMOOTHNESS].Get(), 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

		ResourceStateManager::ChangeResourceStateAndGetBarrier(
			*mGeometryBuffers[GeometryPass::BASECOLOR_METALMASK].Get(), 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

		ResourceStateManager::ChangeResourceStateAndGetBarrier(
			*mDepthBuffer, 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};

	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == GeometryPass::BUFFERS_COUNT + 1UL);
	commandList.ResourceBarrier(barriersCount, barriers);

	commandList.ClearRenderTargetView(mRenderTargetView, DirectX::Colors::Black, 0U, nullptr);

	CHECK_HR(commandList.Close());
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}

void LightingPass::ExecuteFinalTask() noexcept {
	ASSERT(IsDataValid());

	// Check resource states:
	// - All geometry shaders must be in pixel shader resource state because they were used
	// by lighting pass shaders.
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < GeometryPass::BUFFERS_COUNT; ++i) {
		ASSERT(ResourceStateManager::GetResourceState(*mGeometryBuffers[i].Get()) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
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