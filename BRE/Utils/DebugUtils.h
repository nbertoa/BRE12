#pragma once

#include <cassert>
#include <comdef.h>
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
    const HRESULT __hr__ = (x);                                               \
	if (FAILED(__hr__)) { \
		const std::wstring fileName = StringUtils::AnsiToWString(__FILE__); \
		_com_error err(__hr__); \
		const std::wstring lineNumberString = std::to_wstring(__LINE__); \
		const std::wstring errorMessage = err.ErrorMessage(); \
		const std::wstring outputMessage = L" failed in " + fileName + L"; line " + lineNumberString + L"; error: " + errorMessage; \
		MessageBox(0, outputMessage.c_str(), 0, 0); \
		abort(); \
	} \
}
#endif
