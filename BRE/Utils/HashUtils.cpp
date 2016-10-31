#include "HashUtils.h"

#include "DebugUtils.h"

namespace HashUtils {
	std::size_t HashCString(const char* p) noexcept {
		ASSERT(p != nullptr);
		std::size_t hash = 0UL;
		while (*p != '\0') {
			hash ^= *p + 0x9e3779b9 + (hash << 6UL) + (hash >> 2UL);
			++p;
		}

		return hash;
	}
}
