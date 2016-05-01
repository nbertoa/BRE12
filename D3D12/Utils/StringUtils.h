#pragma once

#include <codecvt>
#include <string>
#include <windows.h>

namespace StringUtils {
	inline void ToWideString(const std::string& source, std::wstring& dest) {
		dest.assign(source.begin(), source.end());
	}

	inline std::wstring ToWideString(const std::string& source) {
		std::wstring dest;
		dest.assign(source.begin(), source.end());
		return dest;
	}

	inline std::string ToString(const std::wstring& source) {
		typedef std::codecvt_utf8<wchar_t> convert_type;
		std::wstring_convert<convert_type, wchar_t> converter;
		return converter.to_bytes(source);
	}

	inline std::wstring AnsiToWString(const std::string& str) {
		WCHAR buffer[512U];
		MultiByteToWideChar(CP_ACP, 0U, str.c_str(), -1, buffer, 512);
		return std::wstring(buffer);
	}
}