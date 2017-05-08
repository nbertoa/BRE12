#include "HashUtils.h"

#include "DebugUtils.h"

namespace HashUtils {
std::size_t HashCString(const char* str) noexcept
{
    ASSERT(str != nullptr);
    std::size_t hashValue = 0UL;
    while (*str != '\0') {
        hashValue ^= *str + 0x9e3779b9 + (hashValue << 6UL) + (hashValue >> 2UL);
        ++str;
    }

    return hashValue;
}
}
