#pragma once

#include <cassert>
#include <comdef.h>
#include <cstdlib>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <iostream>
#include <string>

#include <Utils\StringUtils.h>

#if defined(DEBUG) || defined(_DEBUG)
#define ASSERT(condition) \
	assert(condition);
#else
#define ASSERT(condition) (condition)
#endif

#ifndef CHECK_HR
#define CHECK_HR(x) \
{ \
    const HRESULT hr__ = (x);                                               \
	if (FAILED(hr__)) { \
		const std::wstring wfn = StringUtils::AnsiToWString(__FILE__); \
		_com_error err(hr__); \
		const std::wstring msg = err.ErrorMessage(); \
		const std::wstring outputMsg = L" failed in " + wfn + L"; line " + std::to_wstring(__LINE__) + L"; error: " + msg; \
		MessageBox(0, outputMsg.c_str(), 0, 0); \
		abort(); \
	} \
}
#endif

__forceinline void D3dSetDebugName(IDXGIObject* obj, const char* name) noexcept {
	ASSERT(name != nullptr);
	if (obj) {
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

__forceinline void D3dSetDebugName(ID3D12Device* obj, const char* name) noexcept {
	ASSERT(name != nullptr);
	if (obj) {
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

__forceinline void D3dSetDebugName(ID3D12DeviceChild* obj, const char* name) noexcept {
	ASSERT(name != nullptr);
	if (obj) {
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

#ifndef RELEASE_COM
#define RELEASE_COM(x) { if(x){ x->Release(); x = nullptr; } }
#endif
