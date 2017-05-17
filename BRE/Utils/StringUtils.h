#pragma once

#include <string>

namespace BRE {
namespace StringUtils {
///
/// @brief Convert a string to wide string
/// @param source Source string
/// @param destination Destination wide string
///
 void AnsiToWideString(const std::string& source,
                       std::wstring& destination) noexcept;

///
/// @brief Converts ANSI string to wide string
/// @param str Source string
/// @return Wide string
///
std::wstring AnsiToWideString(const std::string& str) noexcept;
}
}

