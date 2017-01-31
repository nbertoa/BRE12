#pragma once

#include <d3d12.h>
#include <mutex>
#include <wrl.h>

#include <Utils/DebugUtils.h>

// To create render target descriptor heaps
// To create render target descriptors
class RenderTargetDescriptorManager {
public:
	RenderTargetDescriptorManager() = delete;
	~RenderTargetDescriptorManager() = delete;
	RenderTargetDescriptorManager(const RenderTargetDescriptorManager&) = delete;
	const RenderTargetDescriptorManager& operator=(const RenderTargetDescriptorManager&) = delete;
	RenderTargetDescriptorManager(RenderTargetDescriptorManager&&) = delete;
	RenderTargetDescriptorManager& operator=(RenderTargetDescriptorManager&&) = delete;

	static void Init() noexcept;

	//
	// The following methods return GPU descriptor handle for the render target views.
	//
	// In the case of methods that creates several views, it returns the GPU desc handle
	// to the first element. As we guarantee all the other views are contiguous, then
	// you can easily build GPU desc handle for other view.
	//
	// You can pass an optional D3D12_CPU_DESCRIPTOR_HANDLE to the first element. 
	// As we guarantee all the other views are contiguous, then
	// you can easily build GPU desc handle for other view.
	//

	static D3D12_GPU_DESCRIPTOR_HANDLE CreateRenderTargetView(
		ID3D12Resource& resource,
		const D3D12_RENDER_TARGET_VIEW_DESC& descriptor,
		D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle = nullptr) noexcept;

	// Preconditions:
	// - "resources" must not be nullptr
	// - "descriptors" must not be nullptr
	// - "descriptorCount" must be greater than zero
	static D3D12_GPU_DESCRIPTOR_HANDLE CreateRenderTargetViews(
		ID3D12Resource* *resources,
		const D3D12_RENDER_TARGET_VIEW_DESC* descriptors,
		const std::uint32_t descriptorCount,
		D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle = nullptr) noexcept;

private:
	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRenderTargetViewDescriptorHeap;

	static D3D12_GPU_DESCRIPTOR_HANDLE mCurrentRenderTargetViewDescriptorHandle;
	static D3D12_CPU_DESCRIPTOR_HANDLE mCurrentRenderTargetViewCpuDescriptorHandle;

	static std::mutex mMutex;
};
