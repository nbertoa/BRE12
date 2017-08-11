#pragma once

#include <cassert>
#include <comdef.h>
#include <string>

#include <Utils\StringUtils.h>

#ifdef _DEBUG
#define BRE_ASSERT(condition) assert(condition)
#else
#define BRE_ASSERT(condition) {}
#endif

#define BRE_CHECK_MSG(condition, msg) \
{ \
	if ((condition) == false) { \
		MessageBox(0, msg, 0, 0); \
		abort(); \
	} \
}

#ifndef BRE_CHECK_HR
#define BRE_CHECK_HR(x) \
{ \
    const HRESULT __hr__ = (x); \
	if (FAILED(__hr__)) { \
		_com_error err(__hr__); \
		const std::wstring errorMessage = err.ErrorMessage(); \
		MessageBox(0, errorMessage.c_str(), 0, 0); \
		abort(); \
	} \
}
#endif
