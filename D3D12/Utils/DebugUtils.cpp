#include "DebugUtils.h"

#include <comdef.h>

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