#include "StringUtils.h"

#include <codecvt>
#include <Windows.h>

namespace BRE {
namespace StringUtils {
void 
ToWideString(const std::string& source,
             std::wstring& destination) noexcept
{
    destination.assign(source.begin(), source.end());
}

std::wstring 
ToWideString(const std::string& source) noexcept
{
    std::wstring dest;
    dest.assign(source.begin(), source.end());
    return dest;
}

std::string 
ToString(const std::wstring& source) noexcept
{
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    return converter.to_bytes(source);
}

std::wstring 
AnsiToWideString(const std::string& str) noexcept
{
    static const std::uint32_t bufferMaxSize = 512U;
    WCHAR buffer[bufferMaxSize];
    MultiByteToWideChar(CP_ACP, 0U, str.c_str(), -1, buffer, bufferMaxSize);
    return std::wstring(buffer);
}
}
}

