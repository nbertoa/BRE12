#pragma once

#include <d3d12.h>
#include <mutex>
#include <wrl.h>

#include <Utils/DebugUtils.h>

// This class is responsible to:
// - Create descriptor heaps
// - Create descriptors
class DescriptorManager {
public:
	static DescriptorManager& Create(ID3D12Device& device) noexcept;
	static DescriptorManager& Get() noexcept;

	~DescriptorManager() = default;
	DescriptorManager(const DescriptorManager&) = delete;
	const DescriptorManager& operator=(const DescriptorManager&) = delete;
	DescriptorManager(DescriptorManager&&) = delete;
	DescriptorManager& operator=(DescriptorManager&&) = delete;
	
	//
	// The following methods return GPU descriptor handle for the CBV, SRV or UAV.
	// In the case of methods that creates several views, it returns the GPU desc handle
	// to the first element. As we guarantee all the other views are contiguous, then
	// you can easily build GPU desc handle for other view.
	//
	
	D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc) noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferViews(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc, const std::uint32_t count) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceView(ID3D12Resource& res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceView(ID3D12Resource* *res, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc, const std::uint32_t count) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE CreateUnorderedAccessView(ID3D12Resource& res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc) noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE CreateUnorderedAccessView(ID3D12Resource* *res, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc, const std::uint32_t count) noexcept;
		
	ID3D12DescriptorHeap& GetCbvSrcUavDescriptorHeap() noexcept {
		ASSERT(mCbvSrvUavDescHeap.Get() != nullptr);
		return *mCbvSrvUavDescHeap.Get();
	}
	
	__forceinline std::size_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE descHeapType) const noexcept {
		return mDevice.GetDescriptorHandleIncrementSize(descHeapType);
	};

private:
	explicit DescriptorManager(ID3D12Device& device);

	ID3D12Device& mDevice;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescHeap;

	D3D12_GPU_DESCRIPTOR_HANDLE mCurrGpuDescHandle{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mCurrCpuDescHandle{ 0UL };

	std::mutex mMutex;
};
