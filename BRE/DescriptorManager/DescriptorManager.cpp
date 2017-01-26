#include "DescriptorManager.h"

#include <memory>

#include <DirectXManager\DirectXManager.h>
#include <DXUtils/d3dx12.h>
#include <SettingsManager\SettingsManager.h>

namespace {
	std::unique_ptr<DescriptorManager> gManager{ nullptr };
}

DescriptorManager& DescriptorManager::Create() noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new DescriptorManager());
	return *gManager.get();
}

DescriptorManager& DescriptorManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

DescriptorManager::DescriptorManager() {
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavDescriptorHeapDescriptor{};
	cbvSrvUavDescriptorHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvSrvUavDescriptorHeapDescriptor.NodeMask = 0U;
	cbvSrvUavDescriptorHeapDescriptor.NumDescriptors = 3000U;
	cbvSrvUavDescriptorHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewDescriptorHeapDescriptor{};
	renderTargetViewDescriptorHeapDescriptor.NumDescriptors = 10U;
	renderTargetViewDescriptorHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewDescriptorHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	renderTargetViewDescriptorHeapDescriptor.NodeMask = 0;

	D3D12_DESCRIPTOR_HEAP_DESC depthStencilViewDescriptorHeapDescriptor{};
	depthStencilViewDescriptorHeapDescriptor.NumDescriptors = 1U;
	depthStencilViewDescriptorHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	depthStencilViewDescriptorHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	depthStencilViewDescriptorHeapDescriptor.NodeMask = 0U;

	mMutex.lock();
	CHECK_HR(DirectXManager::Device().CreateDescriptorHeap(
		&cbvSrvUavDescriptorHeapDescriptor, 
		IID_PPV_ARGS(mCbvSrvUavDescriptorHeap.GetAddressOf())));

	CHECK_HR(DirectXManager::Device().CreateDescriptorHeap(
		&renderTargetViewDescriptorHeapDescriptor, 
		IID_PPV_ARGS(mRenderTargetViewDescriptorHeap.GetAddressOf())));

	CHECK_HR(DirectXManager::Device().CreateDescriptorHeap(
		&depthStencilViewDescriptorHeapDescriptor, 
		IID_PPV_ARGS(mDepthStencilViewDescriptorHeap.GetAddressOf())));
	mMutex.unlock();

	mCurrentCbvSrvUavGpuDescriptorHandle = mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrentCbvSrvUavCpuDescriptorHandle = mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	mCurrentRenderTargetViewDescriptorHandle = mRenderTargetViewDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrentRenderTargetViewCpuDescriptorHandle = mRenderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	mCurrentDepthStencilViewGpuDescriptorHandle = mDepthStencilViewDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrentDepthStencilCpuDescriptorHandle = mDepthStencilViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& descriptor) noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();	
	gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

	DirectXManager::Device().CreateConstantBufferView(&descriptor, mCurrentCbvSrvUavCpuDescriptorHandle);

	mCurrentCbvSrvUavGpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrentCbvSrvUavCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateConstantBufferViews(
	const D3D12_CONSTANT_BUFFER_VIEW_DESC* descriptors, 
	const std::uint32_t descriptorCount) noexcept 
{
	ASSERT(descriptors != nullptr);
	ASSERT(descriptorCount > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();
	gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

	for (std::uint32_t i = 0U; i < descriptorCount; ++i) {
		DirectXManager::Device().CreateConstantBufferView(&descriptors[i], mCurrentCbvSrvUavCpuDescriptorHandle);		
		mCurrentCbvSrvUavCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrentCbvSrvUavGpuDescriptorHandle.ptr += descriptorCount * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateShaderResourceView(
	ID3D12Resource& resource, 
	const D3D12_SHADER_RESOURCE_VIEW_DESC& descriptor) noexcept 
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();	
	gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

	DirectXManager::Device().CreateShaderResourceView(&resource, &descriptor, mCurrentCbvSrvUavCpuDescriptorHandle);

	mCurrentCbvSrvUavGpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrentCbvSrvUavCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateShaderResourceViews(
	ID3D12Resource* *resources, 
	const D3D12_SHADER_RESOURCE_VIEW_DESC* descriptors, 
	const std::uint32_t descriptorCount) noexcept 
{
	ASSERT(resources != nullptr);
	ASSERT(descriptors != nullptr);
	ASSERT(descriptorCount > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();
	gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

	for (std::uint32_t i = 0U; i < descriptorCount; ++i) {
		ASSERT(resources[i] != nullptr);
		DirectXManager::Device().CreateShaderResourceView(resources[i], &descriptors[i], mCurrentCbvSrvUavCpuDescriptorHandle);
		mCurrentCbvSrvUavCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrentCbvSrvUavGpuDescriptorHandle.ptr += descriptorCount * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateUnorderedAccessView(
	ID3D12Resource& resource, 
	const D3D12_UNORDERED_ACCESS_VIEW_DESC& descriptor) noexcept 
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();	
	gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

	DirectXManager::Device().CreateUnorderedAccessView(&resource, nullptr, &descriptor, mCurrentCbvSrvUavCpuDescriptorHandle);

	mCurrentCbvSrvUavGpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrentCbvSrvUavCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateUnorderedAccessViews(
	ID3D12Resource* *resources,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC* descriptors,
	const std::uint32_t descriptorCount) noexcept
{
	ASSERT(resources != nullptr);
	ASSERT(descriptors != nullptr);
	ASSERT(descriptorCount > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();
	gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

	for (std::uint32_t i = 0U; i < descriptorCount; ++i) {
		ASSERT(resources[i] != nullptr);
		DirectXManager::Device().CreateUnorderedAccessView(resources[i], nullptr, &descriptors[i], mCurrentCbvSrvUavCpuDescriptorHandle);
		mCurrentCbvSrvUavCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrentCbvSrvUavGpuDescriptorHandle.ptr += descriptorCount * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateRenderTargetView(
	ID3D12Resource& resource, 
	const D3D12_RENDER_TARGET_VIEW_DESC& descriptor,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptorHandle) noexcept 
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();	
	gpuDescriptorHandle = mCurrentRenderTargetViewDescriptorHandle;

	if (cpuDescriptorHandle != nullptr) {
		*cpuDescriptorHandle = mCurrentRenderTargetViewCpuDescriptorHandle;
	}

	DirectXManager::Device().CreateRenderTargetView(&resource, &descriptor, mCurrentRenderTargetViewCpuDescriptorHandle);

	mCurrentRenderTargetViewDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrentRenderTargetViewCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateRenderTargetViews(
	ID3D12Resource* *resources,
	const D3D12_RENDER_TARGET_VIEW_DESC* descriptors,
	const std::uint32_t descriptorCount,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptorHandle) noexcept
{
	ASSERT(resources != nullptr);
	ASSERT(descriptors != nullptr);
	ASSERT(descriptorCount > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();
	gpuDescriptorHandle = mCurrentRenderTargetViewDescriptorHandle;

	if (cpuDescriptorHandle != nullptr) {
		*cpuDescriptorHandle = mCurrentRenderTargetViewCpuDescriptorHandle;
	}

	for (std::uint32_t i = 0U; i < descriptorCount; ++i) {
		ASSERT(resources[i] != nullptr);
		DirectXManager::Device().CreateRenderTargetView(resources[i], &descriptors[i], mCurrentRenderTargetViewCpuDescriptorHandle);
		mCurrentRenderTargetViewCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrentRenderTargetViewDescriptorHandle.ptr += descriptorCount * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateDepthStencilView(
	ID3D12Resource& resource,
	const D3D12_DEPTH_STENCIL_VIEW_DESC& descriptor,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptorHandle) noexcept 
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();
	gpuDescriptorHandle = mCurrentDepthStencilViewGpuDescriptorHandle;

	if (cpuDescriptorHandle != nullptr) {
		*cpuDescriptorHandle = mCurrentDepthStencilCpuDescriptorHandle;
	}

	DirectXManager::Device().CreateDepthStencilView(&resource, &descriptor, mCurrentDepthStencilCpuDescriptorHandle);

	mCurrentDepthStencilViewGpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrentDepthStencilCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateDepthStencilViews(
	ID3D12Resource* *resources,
	const D3D12_DEPTH_STENCIL_VIEW_DESC* descriptors,
	const std::uint32_t descriptorCount,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptorHandle) noexcept
{
	ASSERT(resources != nullptr);
	ASSERT(descriptors != nullptr);
	ASSERT(descriptorCount > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

	mMutex.lock();
	gpuDescriptorHandle = mCurrentDepthStencilViewGpuDescriptorHandle;

	if (cpuDescriptorHandle != nullptr) {
		*cpuDescriptorHandle = mCurrentDepthStencilCpuDescriptorHandle;
	}

	for (std::uint32_t i = 0U; i < descriptorCount; ++i) {
		ASSERT(resources[i] != nullptr);
		DirectXManager::Device().CreateDepthStencilView(resources[i], &descriptors[i], mCurrentDepthStencilCpuDescriptorHandle);
		mCurrentDepthStencilCpuDescriptorHandle.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrentDepthStencilViewGpuDescriptorHandle.ptr += descriptorCount * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescriptorHandle;
}