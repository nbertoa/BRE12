#pragma once

#include <string>

namespace BRE {
namespace StringUtils {
///
/// @brief Convert a string to wide string
/// @param source Source string
/// @param destination Destination wide string
///
 void ToWideString(const std::string& source,
                   std::wstring& destination) noexcept;

///
/// @brief Converts string to wide string
/// @param source Source string
/// @return Wide string
///
std::wstring ToWideString(const std::string& source) noexcept;

///
/// @brief Converts wide string to string
/// @param source Source wide string
/// @return Converted string
///
std::string ToString(const std::wstring& source) noexcept;

///
/// @brief Converts ANSI string to wide string
/// @param str Source string
/// @return Wide string
///
std::wstring AnsiToWideString(const std::string& str) noexcept;
}
}

