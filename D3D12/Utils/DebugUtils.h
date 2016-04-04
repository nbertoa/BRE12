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
#define ASSERT(condition);
#endif

#if defined(_DEBUG) || defined(DEBUG)
#ifndef ASSERT_HR
#define ASSERT_HR(x){																\
		const HRESULT hr = (x);														\
		if(FAILED(hr)){															\
			std::cout << "An error occured on line" << (DWORD)__LINE__ << " in the file " << __FILE__ << std::endl; \
			std::cout << hr << std::endl; \
			abort(); \
		}																		\
	}
#endif
#else
#ifndef ASSERT_HR
#define ASSERT_HR(x) (x)
#endif
#endif

inline void D3dSetDebugName(IDXGIObject* obj, const char* name) {
	if (obj) {
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

inline void D3dSetDebugName(ID3D12Device* obj, const char* name) {
	if (obj) {
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

inline void D3dSetDebugName(ID3D12DeviceChild* obj, const char* name) {
	if (obj) {
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

class DxException {
public:
	DxException() = default;
	DxException::DxException(const HRESULT hr, const std::wstring& functionName, const std::wstring& filename, const int32_t lineNumber)
		: mErrorCode(hr)
		, mFunctionName(functionName)
		, mFilename(filename)
		, mLineNumber(lineNumber)
	{
	}

	std::wstring DxException::ToString() const {
		// Get the string description of the error code.
		_com_error err(mErrorCode);
		const std::wstring msg = err.ErrorMessage();
		return mFunctionName + L" failed in " + mFilename + L"; line " + std::to_wstring(mLineNumber) + L"; error: " + msg;
	}

	HRESULT mErrorCode = S_OK;
	std::wstring mFunctionName = L"";
	std::wstring mFilename = L"";
	int mLineNumber = -1;
};
