#include "DepthStencilDescriptorManager.h"

#include <memory>

#include <DirectXManager\DirectXManager.h>
#include <DXUtils/d3dx12.h>
#include <SettingsManager\SettingsManager.h>

namespace {
	std::unique_ptr<DepthStencilDescriptorManager> gManager{ nullptr };
}

DepthStencilDescriptorManager& DepthStencilDescriptorManager::Create() noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new DepthStencilDescriptorManager());
	return *gManager.get();
}

DepthStencilDescriptorManager& DepthStencilDescriptorManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

DepthStencilDescriptorManager::DepthStencilDescriptorManager() {
	D3D12_DESCRIPTOR_HEAP_DESC depthStencilViewDescriptorHeapDescriptor{};
	depthStencilViewDescriptorHeapDescriptor.NumDescriptors = 1U;
	depthStencilViewDescriptorHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	depthStencilViewDescriptorHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	depthStencilViewDescriptorHeapDescriptor.NodeMask = 0U;

	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateDescriptorHeap(
		&depthStencilViewDescriptorHeapDescriptor,
		IID_PPV_ARGS(mDepthStencilViewDescriptorHeap.GetAddressOf())));
	mMutex.unlock();

	mCurrentDepthStencilViewGpuDescriptorHandle = mDepthStencilViewDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrentDepthStencilCpuDescriptorHandle = mDepthStencilViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE DepthStencilDescriptorManager::CreateDepthStencilView(
	ID3D12Resource& resource,
	const D3D12_DEPTH_STENCIL_VIEW_DESC& descriptor,
	D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle) noexcept
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();
	gpuDescriptorHandle = mCurrentDepthStencilViewGpuDescriptorHandle;

	if (firstViewCpuDescriptorHandle != nullptr) {
		*firstViewCpuDescriptorHandle = mCurrentDepthStencilCpuDescriptorHandle;
	}

	DirectXManager::GetDevice().CreateDepthStencilView(&resource, &descriptor, mCurrentDepthStencilCpuDescriptorHandle);

	mCurrentDepthStencilViewGpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrentDepthStencilCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DepthStencilDescriptorManager::CreateDepthStencilViews(
	ID3D12Resource* *resources,
	const D3D12_DEPTH_STENCIL_VIEW_DESC* descriptors,
	const std::uint32_t descriptorCount,
	D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle) noexcept
{
	ASSERT(resources != nullptr);
	ASSERT(descriptors != nullptr);
	ASSERT(descriptorCount > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();
	gpuDescriptorHandle = mCurrentDepthStencilViewGpuDescriptorHandle;

	if (firstViewCpuDescriptorHandle != nullptr) {
		*firstViewCpuDescriptorHandle = mCurrentDepthStencilCpuDescriptorHandle;
	}

	for (std::uint32_t i = 0U; i < descriptorCount; ++i) {
		ASSERT(resources[i] != nullptr);
		DirectXManager::GetDevice().CreateDepthStencilView(resources[i], &descriptors[i], mCurrentDepthStencilCpuDescriptorHandle);
		mCurrentDepthStencilCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrentDepthStencilViewGpuDescriptorHandle.ptr += descriptorCount * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescriptorHandle;
}