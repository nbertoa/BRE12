#pragma once

#include <codecvt>

namespace BRE {
namespace StringUtils {
///
/// @brief Convert a string to wide string
/// @param source Source string
/// @param destination Destination wide string
///
__forceinline void ToWideString(const std::string& source,
                                std::wstring& destination) noexcept
{
    destination.assign(source.begin(), source.end());
}

///
/// @brief Converts string to wide string
/// @param source Source string
/// @return Wide string
///
inline std::wstring ToWideString(const std::string& source) noexcept
{
    std::wstring dest;
    dest.assign(source.begin(), source.end());
    return dest;
}

///
/// @brief Converts wide string to string
/// @param source Source wide string
/// @return Converted string
///
inline std::string ToString(const std::wstring& source) noexcept
{
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    return converter.to_bytes(source);
}

///
/// @brief Converts ANSI string to wide string
/// @param str Source string
/// @return Wide string
///
inline std::wstring AnsiToWideString(const std::string& str) noexcept
{
    static const std::uint32_t bufferMaxSize = 512U;
    WCHAR buffer[bufferMaxSize];
    MultiByteToWideChar(CP_ACP, 0U, str.c_str(), -1, buffer, bufferMaxSize);
    return std::wstring(buffer);
}
}
}

