#pragma once

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include <Utils\DebugUtils.h>

class D3dData {
public:
	D3dData() = delete;
	~D3dData() = delete;
	D3dData(const D3dData&) = delete;
	const D3dData& operator=(const D3dData&) = delete;
	D3dData(D3dData&&) = delete;
	D3dData& operator=(D3dData&&) = delete;

	static void InitDirect3D(const HINSTANCE hInstance) noexcept;
	
	__forceinline static HWND Hwnd() noexcept { return mHwnd; }
	__forceinline static IDXGIFactory4& Factory() noexcept { ASSERT(mDxgiFactory.Get() != nullptr); return *mDxgiFactory.Get(); }
	__forceinline static ID3D12Device& Device() noexcept { ASSERT(mDevice.Get() != nullptr); return *mDevice.Get(); }
	__forceinline static std::size_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE descHeapType) {
		ASSERT(mDevice.Get() != nullptr);
		return mDevice.Get()->GetDescriptorHandleIncrementSize(descHeapType);
	}

private:
	static HWND mHwnd;
	static Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
	static Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
};
