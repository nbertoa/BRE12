#pragma once

#include <d3d12.h>
#include <mutex>
#include <wrl.h>

#include <Utils/DebugUtils.h>

// To create depth stencil descriptor heaps
// To create depth stencil descriptors
class DepthStencilDescriptorManager {
public:

	// Preconditions:
	// - Create() must be called once
	static DepthStencilDescriptorManager& Create() noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static DepthStencilDescriptorManager& Get() noexcept;

	~DepthStencilDescriptorManager() = default;
	DepthStencilDescriptorManager(const DepthStencilDescriptorManager&) = delete;
	const DepthStencilDescriptorManager& operator=(const DepthStencilDescriptorManager&) = delete;
	DepthStencilDescriptorManager(DepthStencilDescriptorManager&&) = delete;
	DepthStencilDescriptorManager& operator=(DepthStencilDescriptorManager&&) = delete;

	//
	// The following methods return GPU descriptor handle for the depth stencil view.
	//
	// In the case of methods that creates several views, it returns the GPU desc handle
	// to the first element. As we guarantee all the other views are contiguous, then
	// you can easily build GPU desc handle for other view.
	//
	// Yu can pass an optional D3D12_CPU_DESCRIPTOR_HANDLE to the first element. 
	// As we guarantee all the other views are contiguous, then
	// you can easily build GPU desc handle for other view.
	//

	D3D12_GPU_DESCRIPTOR_HANDLE CreateDepthStencilView(
		ID3D12Resource& resource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC& descriptor,
		D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle = nullptr) noexcept;

	// Preconditions:
	// - "resources" must not be nullptr
	// - "descriptors" must not be nullptr
	// - "descriptorCount" must be greater than zero
	D3D12_GPU_DESCRIPTOR_HANDLE CreateDepthStencilViews(
		ID3D12Resource* *resources,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* descriptors,
		const std::uint32_t descriptorCount,
		D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle = nullptr) noexcept;

private:
	DepthStencilDescriptorManager();

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDepthStencilViewDescriptorHeap;

	D3D12_GPU_DESCRIPTOR_HANDLE mCurrentDepthStencilViewGpuDescriptorHandle{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mCurrentDepthStencilCpuDescriptorHandle{ 0UL };

	std::mutex mMutex;
};
