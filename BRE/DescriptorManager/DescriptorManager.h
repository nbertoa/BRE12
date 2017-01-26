#pragma once

#include <d3d12.h>
#include <mutex>
#include <wrl.h>

#include <Utils/DebugUtils.h>

// To create descriptor heaps
// To create descriptors
class DescriptorManager {
public:

	// Preconditions:
	// - Create() must be called once
	static DescriptorManager& Create() noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static DescriptorManager& Get() noexcept;

	~DescriptorManager() = default;
	DescriptorManager(const DescriptorManager&) = delete;
	const DescriptorManager& operator=(const DescriptorManager&) = delete;
	DescriptorManager(DescriptorManager&&) = delete;
	DescriptorManager& operator=(DescriptorManager&&) = delete;
	
	//
	// The following methods return GPU descriptor handle for the CBV, SRV or UAV.
	//
	// In the case of methods that creates several views, it returns the GPU desc handle
	// to the first element. As we guarantee all the other views are contiguous, then
	// you can easily build GPU desc handle for other view.
	//
	// In the case of render target / depth stencil view methods, you can pass an optional
	// D3D12_CPU_DESCRIPTOR_HANDLE to the first element. 
	// As we guarantee all the other views are contiguous, then
	// you can easily build GPU desc handle for other view.
	//
	
	D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& cBufferViewDescriptor) noexcept;

	// Preconditions:
	// - "descriptors" must not be nullptr
	// - "descriptorCount" must be greated than zero
	D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferViews(
		const D3D12_CONSTANT_BUFFER_VIEW_DESC* descriptors,
		const std::uint32_t descriptorCount) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceView(
		ID3D12Resource& resource, 
		const D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceViewDescriptor) noexcept;

	// Preconditions:
	// - "resources" must not be nullptr
	// - "descriptors" must not be nullptr
	// - "descriptorCount" must be greated than zero
	D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceViews(
		ID3D12Resource* *resources, 
		const D3D12_SHADER_RESOURCE_VIEW_DESC* descriptors,
		const std::uint32_t descriptorCount) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE CreateUnorderedAccessView(
		ID3D12Resource& resource, 
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& descriptor) noexcept;

	// Preconditions:
	// - "resources" must not be nullptr
	// - "descriptors" must not be nullptr
	// - "descriptorCount" must be greated than zero
	D3D12_GPU_DESCRIPTOR_HANDLE CreateUnorderedAccessViews(
		ID3D12Resource* *resources, 
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* descriptors,
		const std::uint32_t descriptorCount) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE CreateRenderTargetView(
		ID3D12Resource& resource, 
		const D3D12_RENDER_TARGET_VIEW_DESC& descriptor, 
		D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptorHandle = nullptr) noexcept;

	// Preconditions:
	// - "resources" must not be nullptr
	// - "descriptors" must not be nullptr
	// - "descriptorCount" must be greater than zero
	D3D12_GPU_DESCRIPTOR_HANDLE CreateRenderTargetViews(
		ID3D12Resource* *resources, 
		const D3D12_RENDER_TARGET_VIEW_DESC* descriptors,
		const std::uint32_t descriptorCount,
		D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptorHandle = nullptr) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE CreateDepthStencilView(
		ID3D12Resource& resource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC& descriptor,
		D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptorHandle = nullptr) noexcept;

	// Preconditions:
	// - "resources" must not be nullptr
	// - "descriptors" must not be nullptr
	// - "descriptorCount" must be greater than zero
	D3D12_GPU_DESCRIPTOR_HANDLE CreateDepthStencilViews(
		ID3D12Resource* *resources,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* descriptors,
		const std::uint32_t descriptorCount,
		D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptorHandle = nullptr) noexcept;
		
	ID3D12DescriptorHeap& GetCbvSrcUavDescriptorHeap() noexcept {
		ASSERT(mCbvSrvUavDescriptorHeap.Get() != nullptr);
		return *mCbvSrvUavDescriptorHeap.Get();
	}

private:
	DescriptorManager();

	// Descriptor heaps for:
	// - Constant Buffer View - Shader Resource View - Unordered Access View
	// - Render Target View
	// - Depth Stencil View
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRenderTargetViewDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDepthStencilViewDescriptorHeap;

	D3D12_GPU_DESCRIPTOR_HANDLE mCurrentCbvSrvUavGpuDescriptorHandle{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mCurrentCbvSrvUavCpuDescriptorHandle{ 0UL };

	D3D12_GPU_DESCRIPTOR_HANDLE mCurrentRenderTargetViewDescriptorHandle{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mCurrentRenderTargetViewCpuDescriptorHandle{ 0UL };

	D3D12_GPU_DESCRIPTOR_HANDLE mCurrentDepthStencilViewGpuDescriptorHandle{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mCurrentDepthStencilCpuDescriptorHandle{ 0UL };

	std::mutex mMutex;
};
