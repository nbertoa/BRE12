#include "DescriptorManager.h"

#include <memory>

#include <DXUtils/d3dx12.h>

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
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = 3000U;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	mMutex.lock();
	CHECK_HR(mDevice.CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(mCbvSrvUavDescHeap.GetAddressOf())));
	mMutex.unlock();

	mCurrGpuDescHandle = mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrCpuDescHandle = mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc) noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();
	mDevice.CreateConstantBufferView(&desc, mCurrCpuDescHandle);
	gpuDescHandle = mCurrGpuDescHandle;
	mCurrGpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateConstantBufferViews(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc, const std::uint32_t count) noexcept {
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();
	gpuDescHandle = mCurrGpuDescHandle;

	for (std::uint32_t i = 0U; i < count; ++i) {
		mDevice.CreateConstantBufferView(&desc[i], mCurrCpuDescHandle);		
		mCurrCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrGpuDescHandle.ptr += count * GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateShaderResourceView(ID3D12Resource& res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();
	mDevice.CreateShaderResourceView(&res, &desc, mCurrCpuDescHandle);
	gpuDescHandle = mCurrGpuDescHandle;
	mCurrGpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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
	gpuDescHandle = mCurrGpuDescHandle;

	for (std::uint32_t i = 0U; i < count; ++i) {
		ASSERT(res[i] != nullptr);
		mDevice.CreateShaderResourceView(res[i], &desc[i], mCurrCpuDescHandle);
		mCurrCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrGpuDescHandle.ptr += count * GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateUnorderedAccessView(ID3D12Resource& res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc) noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle{};

	mMutex.lock();
	mDevice.CreateUnorderedAccessView(&res, nullptr, &desc, mCurrCpuDescHandle);
	gpuDescHandle = mCurrGpuDescHandle;
	mCurrGpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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
	gpuDescHandle = mCurrGpuDescHandle;

	for (std::uint32_t i = 0U; i < count; ++i) {
		ASSERT(res[i] != nullptr);
		mDevice.CreateUnorderedAccessView(res[i], nullptr, &desc[i], mCurrCpuDescHandle);
		mCurrCpuDescHandle.ptr += GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrGpuDescHandle.ptr += count * GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDescHandle;
}