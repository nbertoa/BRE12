#include "StringUtils.h"

#include <codecvt>
#include <Windows.h>

namespace BRE {
namespace StringUtils {
void
AnsiToWideString(const std::string& source,
                 std::wstring& destination) noexcept
{
    destination.assign(source.begin(), source.end());
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