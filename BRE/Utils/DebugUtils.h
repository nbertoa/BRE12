#pragma once

#include <cassert>
#include <comdef.h>
#include <string>

#include <Utils\StringUtils.h>

#define ASSERT(condition) assert(condition)

#if defined(DEBUG) || defined(_DEBUG)
#define ASSERT_MSG(condition, msg) \
{ \
	if ((condition) == false) { \
		MessageBox(0, msg, 0, 0); \
		abort(); \
	} \
}
#else
#define ASSERT_MSG(condition, msg) (assert(condition))
#endif

#ifndef CHECK_HR
#define CHECK_HR(x) \
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
