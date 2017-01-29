#pragma once

#include <d3d12.h>
#include <mutex>
#include <wrl.h>

#include <Utils/DebugUtils.h>

// To create depth stencil descriptor heaps
// To create depth stencil descriptors
class DepthStencilDescriptorManager {
public:
	DepthStencilDescriptorManager() = delete;
	~DepthStencilDescriptorManager() = delete;
	DepthStencilDescriptorManager(const DepthStencilDescriptorManager&) = delete;
	const DepthStencilDescriptorManager& operator=(const DepthStencilDescriptorManager&) = delete;
	DepthStencilDescriptorManager(DepthStencilDescriptorManager&&) = delete;
	DepthStencilDescriptorManager& operator=(DepthStencilDescriptorManager&&) = delete;

	static void Init() noexcept;

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

	static D3D12_GPU_DESCRIPTOR_HANDLE CreateDepthStencilView(
		ID3D12Resource& resource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC& descriptor,
		D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle = nullptr) noexcept;

	// Preconditions:
	// - "resources" must not be nullptr
	// - "descriptors" must not be nullptr
	// - "descriptorCount" must be greater than zero
	static D3D12_GPU_DESCRIPTOR_HANDLE CreateDepthStencilViews(
		ID3D12Resource* *resources,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* descriptors,
		const std::uint32_t descriptorCount,
		D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle = nullptr) noexcept;

private:
	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDepthStencilViewDescriptorHeap;

	static D3D12_GPU_DESCRIPTOR_HANDLE mCurrentDepthStencilViewGpuDescriptorHandle;
	static D3D12_CPU_DESCRIPTOR_HANDLE mCurrentDepthStencilCpuDescriptorHandle;

	static std::mutex mMutex;
};
