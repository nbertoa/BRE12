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
	CHECK_HR(DirectXManager::Device().CreateDescriptorHeap(&cbvSrvUavDescHeapDesc, IID_PPV_ARGS(mCbvSrvUavDescHeap.GetAddressOf())));
	CHECK_HR(DirectXManager::Device().CreateDescriptorHeap(&rtvDescHeapDesc, IID_PPV_ARGS(mRtvDescHeap.GetAddressOf())));
	CHECK_HR(DirectXManager::Device().CreateDescriptorHeap(&dsvDescHeapDesc, IID_PPV_ARGS(mDsvDescHeap.GetAddressOf())));
	mMutex.unlock();

	mCurrCbvSrvUavGpuDesc = mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrCbvSrvUavCpuDesc = mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart();

	mCurrRtvGpuDesc = mRtvDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrRtvCpuDesc = mRtvDescHeap->GetCPUDescriptorHandleForHeapStart();

	mCurrDsvGpuDesc = mDsvDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCurrDsvCpuDesc = mDsvDescHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc) noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc{};

	mMutex.lock();	
	gpuDesc = mCurrCbvSrvUavGpuDesc;

	DirectXManager::Device().CreateConstantBufferView(&desc, mCurrCbvSrvUavCpuDesc);

	mCurrCbvSrvUavGpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrCbvSrvUavCpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDesc;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateConstantBufferViews(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc, const std::uint32_t count) noexcept {
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc{};

	mMutex.lock();
	gpuDesc = mCurrCbvSrvUavGpuDesc;

	for (std::uint32_t i = 0U; i < count; ++i) {
		DirectXManager::Device().CreateConstantBufferView(&desc[i], mCurrCbvSrvUavCpuDesc);		
		mCurrCbvSrvUavCpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrCbvSrvUavGpuDesc.ptr += count * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDesc;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateShaderResourceView(ID3D12Resource& res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc{};

	mMutex.lock();	
	gpuDesc = mCurrCbvSrvUavGpuDesc;

	DirectXManager::Device().CreateShaderResourceView(&res, &desc, mCurrCbvSrvUavCpuDesc);

	mCurrCbvSrvUavGpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrCbvSrvUavCpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDesc;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateShaderResourceView(
	ID3D12Resource* *res, 
	const D3D12_SHADER_RESOURCE_VIEW_DESC* desc, 
	const std::uint32_t count) noexcept {

	ASSERT(res != nullptr);
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc{};

	mMutex.lock();
	gpuDesc = mCurrCbvSrvUavGpuDesc;

	for (std::uint32_t i = 0U; i < count; ++i) {
		ASSERT(res[i] != nullptr);
		DirectXManager::Device().CreateShaderResourceView(res[i], &desc[i], mCurrCbvSrvUavCpuDesc);
		mCurrCbvSrvUavCpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrCbvSrvUavGpuDesc.ptr += count * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDesc;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateUnorderedAccessView(ID3D12Resource& res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc) noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc{};

	mMutex.lock();	
	gpuDesc = mCurrCbvSrvUavGpuDesc;

	DirectXManager::Device().CreateUnorderedAccessView(&res, nullptr, &desc, mCurrCbvSrvUavCpuDesc);

	mCurrCbvSrvUavGpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrCbvSrvUavCpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDesc;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateUnorderedAccessView(
	ID3D12Resource* *res,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,
	const std::uint32_t count) noexcept
{
	ASSERT(res != nullptr);
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc{};

	mMutex.lock();
	gpuDesc = mCurrCbvSrvUavGpuDesc;

	for (std::uint32_t i = 0U; i < count; ++i) {
		ASSERT(res[i] != nullptr);
		DirectXManager::Device().CreateUnorderedAccessView(res[i], nullptr, &desc[i], mCurrCbvSrvUavCpuDesc);
		mCurrCbvSrvUavCpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrCbvSrvUavGpuDesc.ptr += count * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDesc;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateRenderTargetView(
	ID3D12Resource& res, 
	const D3D12_RENDER_TARGET_VIEW_DESC& desc,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDesc) noexcept {

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc{};

	mMutex.lock();	
	gpuDesc = mCurrRtvGpuDesc;

	if (cpuDesc != nullptr) {
		*cpuDesc = mCurrRtvCpuDesc;
	}

	DirectXManager::Device().CreateRenderTargetView(&res, &desc, mCurrRtvCpuDesc);

	mCurrRtvGpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrRtvCpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDesc;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateRenderTargetViews(
	ID3D12Resource* *res,
	const D3D12_RENDER_TARGET_VIEW_DESC* desc,
	const std::uint32_t count,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDesc) noexcept
{
	ASSERT(res != nullptr);
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc{};

	mMutex.lock();
	gpuDesc = mCurrRtvGpuDesc;

	if (cpuDesc != nullptr) {
		*cpuDesc = mCurrRtvCpuDesc;
	}

	for (std::uint32_t i = 0U; i < count; ++i) {
		ASSERT(res[i] != nullptr);
		DirectXManager::Device().CreateRenderTargetView(res[i], &desc[i], mCurrRtvCpuDesc);
		mCurrRtvCpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrRtvGpuDesc.ptr += count * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDesc;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateDepthStencilView(
	ID3D12Resource& res,
	const D3D12_DEPTH_STENCIL_VIEW_DESC& desc,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDesc) noexcept {

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc{};

	mMutex.lock();
	gpuDesc = mCurrDsvGpuDesc;

	if (cpuDesc != nullptr) {
		*cpuDesc = mCurrDsvCpuDesc;
	}

	DirectXManager::Device().CreateDepthStencilView(&res, &desc, mCurrDsvCpuDesc);

	mCurrDsvGpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrDsvCpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMutex.unlock();

	return gpuDesc;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::CreateDepthStencilViews(
	ID3D12Resource* *res,
	const D3D12_DEPTH_STENCIL_VIEW_DESC* desc,
	const std::uint32_t count,
	D3D12_CPU_DESCRIPTOR_HANDLE* cpuDesc) noexcept
{
	ASSERT(res != nullptr);
	ASSERT(desc != nullptr);
	ASSERT(count > 0U);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc{};

	mMutex.lock();
	gpuDesc = mCurrDsvGpuDesc;

	if (cpuDesc != nullptr) {
		*cpuDesc = mCurrDsvCpuDesc;
	}

	for (std::uint32_t i = 0U; i < count; ++i) {
		ASSERT(res[i] != nullptr);
		DirectXManager::Device().CreateDepthStencilView(res[i], &desc[i], mCurrDsvCpuDesc);
		mCurrDsvCpuDesc.ptr += DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	mCurrDsvGpuDesc.ptr += count * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mMutex.unlock();

	return gpuDesc;
}