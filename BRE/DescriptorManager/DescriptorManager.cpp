#include "DescriptorManager.h"

#include <memory>

#include <DXUtils/d3dx12.h>
#include <GlobalData\Settings.h>

namespace {
	std::unique_ptr<DescriptorManager> gManager{ nullptr };
}

DescriptorManager& DescriptorManager::Create(ID3D12Device& device) noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new DescriptorManager(device));
	return *gManager.get();
}

DescriptorManager& DescriptorManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

DescriptorManager::DescriptorManager(ID3D12Device& device)
	: mDevice(device)
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavDescHeapDesc{};
	cbvSrvUavDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvSrvUavDescHeapDesc.NodeMask = 0U;
	cbvSrvUavDescHeapDesc.NumDescriptors = 3000U;
	cbvSrvUavDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	D3D12_DESCRIPTOR_HEAP_DESC rtvDescHeapDesc{};
	rtvDescHeapDesc.NumDescriptors = 10U;
	rtvDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescHeapDesc.NodeMask = 0;

	D3D12_DESCRIPTOR_HEAP_DESC dsvDescHeapDesc{};
	dsvDescHeapDesc.NumDescriptors = 1U;
	dsvDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescHeapDesc.NodeMask = 0U;

	mMutex.lock();
	CHECK_HR(mDevice.CreateDescriptorHeap(&cbvSrvUavDescHeapDesc, IID_PPV_ARGS(mCbvSrvUavDescHeap.GetAddressOf())));
	CHECK_HR(mDevice.CreateDescriptorHeap(&rtvDescHeapDesc, IID_PPV_ARGS(mRtvDescHeap.GetAddressOf())));
	CHECK_HR(mDevice.CreateDescriptorHeap(&dsvDescHeapDesc, IID_PPV_ARGS(mDsvDescHeap.GetAddressOf())));
	mMutex.unlock();

	mCurrCbvSrvUavGpuDescHandle = mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrCbvSrvUavCpuDescHandle = mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart();

	mCurrRtvGpuDescHandle = mRtvDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrRtvCpuDescHandle = mRtvDescHeap->GetCPUDescriptorHandleForHeapStart();

	mCurrDsvGpuDescHandle = mDsvDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrDsvCpuDescHandle = mDsvDescHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc) noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();	
	gpuDescHandle = mCurrCbvSrvUavGpuDescHandle;

	mDevice.CreateConstantBufferView(&desc, mCurrCbvSrvUavCpuDescHandle);

	mCurrCbvSrvUavGpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrCbvSrvUavCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateConstantBufferViews(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc, const std::uint32_t count) noexcept {
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();
	gpuDescHandle = mCurrCbvSrvUavGpuDescHandle;

	for (std::uint32_t i = 0U; i < count; ++i) {
		mDevice.CreateConstantBufferView(&desc[i], mCurrCbvSrvUavCpuDescHandle);		
		mCurrCbvSrvUavCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrCbvSrvUavGpuDescHandle.ptr += count * GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateShaderResourceView(ID3D12Resource& res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();	
	gpuDescHandle = mCurrCbvSrvUavGpuDescHandle;

	mDevice.CreateShaderResourceView(&res, &desc, mCurrCbvSrvUavCpuDescHandle);

	mCurrCbvSrvUavGpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrCbvSrvUavCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateShaderResourceView(
	ID3D12Resource* *res, 
	const D3D12_SHADER_RESOURCE_VIEW_DESC* desc, 
	const std::uint32_t count) noexcept {

	ASSERT(res != nullptr);
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();
	gpuDescHandle = mCurrCbvSrvUavGpuDescHandle;

	for (std::uint32_t i = 0U; i < count; ++i) {
		ASSERT(res[i] != nullptr);
		mDevice.CreateShaderResourceView(res[i], &desc[i], mCurrCbvSrvUavCpuDescHandle);
		mCurrCbvSrvUavCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrCbvSrvUavGpuDescHandle.ptr += count * GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateUnorderedAccessView(ID3D12Resource& res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc) noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();	
	gpuDescHandle = mCurrCbvSrvUavGpuDescHandle;

	mDevice.CreateUnorderedAccessView(&res, nullptr, &desc, mCurrCbvSrvUavCpuDescHandle);

	mCurrCbvSrvUavGpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrCbvSrvUavCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateUnorderedAccessView(
	ID3D12Resource* *res,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,
	const std::uint32_t count) noexcept
{
	ASSERT(res != nullptr);
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();
	gpuDescHandle = mCurrCbvSrvUavGpuDescHandle;

	for (std::uint32_t i = 0U; i < count; ++i) {
		ASSERT(res[i] != nullptr);
		mDevice.CreateUnorderedAccessView(res[i], nullptr, &desc[i], mCurrCbvSrvUavCpuDescHandle);
		mCurrCbvSrvUavCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrCbvSrvUavGpuDescHandle.ptr += count * GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateRenderTargetView(
	ID3D12Resource& res, 
	const D3D12_RENDER_TARGET_VIEW_DESC& desc,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescHandle) noexcept {

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();	
	gpuDescHandle = mCurrRtvGpuDescHandle;

	if (cpuDescHandle != nullptr) {
		*cpuDescHandle = mCurrRtvCpuDescHandle;
	}

	mDevice.CreateRenderTargetView(&res, &desc, mCurrRtvCpuDescHandle);

	mCurrRtvGpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrRtvCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateRenderTargetViews(
	ID3D12Resource* *res,
	const D3D12_RENDER_TARGET_VIEW_DESC* desc,
	const std::uint32_t count,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescHandle) noexcept
{
	ASSERT(res != nullptr);
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();
	gpuDescHandle = mCurrRtvGpuDescHandle;

	if (cpuDescHandle != nullptr) {
		*cpuDescHandle = mCurrRtvCpuDescHandle;
	}

	for (std::uint32_t i = 0U; i < count; ++i) {
		ASSERT(res[i] != nullptr);
		mDevice.CreateRenderTargetView(res[i], &desc[i], mCurrRtvCpuDescHandle);
		mCurrRtvCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrRtvGpuDescHandle.ptr += count * GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateDepthStencilView(
	ID3D12Resource& res,
	const D3D12_DEPTH_STENCIL_VIEW_DESC& desc,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescHandle) noexcept {

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();
	gpuDescHandle = mCurrDsvGpuDescHandle;

	if (cpuDescHandle != nullptr) {
		*cpuDescHandle = mCurrDsvCpuDescHandle;
	}

	mDevice.CreateDepthStencilView(&res, &desc, mCurrDsvCpuDescHandle);

	mCurrDsvGpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrDsvCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateDepthStencilViews(
	ID3D12Resource* *res,
	const D3D12_DEPTH_STENCIL_VIEW_DESC* desc,
	const std::uint32_t count,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescHandle) noexcept
{
	ASSERT(res != nullptr);
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();
	gpuDescHandle = mCurrDsvGpuDescHandle;

	if (cpuDescHandle != nullptr) {
		*cpuDescHandle = mCurrDsvCpuDescHandle;
	}

	for (std::uint32_t i = 0U; i < count; ++i) {
		ASSERT(res[i] != nullptr);
		mDevice.CreateDepthStencilView(res[i], &desc[i], mCurrDsvCpuDescHandle);
		mCurrDsvCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrDsvGpuDescHandle.ptr += count * GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescHandle;
}