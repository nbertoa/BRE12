#pragma once

#include <string>

void ToWideString(const std::string& source, std::wstring& dest);
std::wstring ToWideString(const std::string& source);
std::string ToString(const std::wstring& source);