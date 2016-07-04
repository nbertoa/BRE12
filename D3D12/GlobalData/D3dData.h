#pragma once

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include <Utils\DebugUtils.h>

class D3dData {
public:
	D3dData() = delete;
	D3dData(const D3dData&) = delete;
	const D3dData& operator=(const D3dData&) = delete;

	static void InitDirect3D() noexcept;
	static void CreateSwapChain(const HWND hwnd, ID3D12CommandQueue& cmdQueue) noexcept;

	__forceinline static std::uint32_t CurrentBackBufferIndex() noexcept { ASSERT(mSwapChain.Get() != nullptr); return mSwapChain->GetCurrentBackBufferIndex(); };

	static Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
	static Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain;
	static Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
};
