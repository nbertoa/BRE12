#pragma once

#include <cstddef>

namespace NumberGeneration {
	// Not thread safe
	std::size_t GetRandomSizeT() noexcept;

	// Thread safe method to get size_t values from 1 to SIZE_T_MAX.
	std::size_t GetIncrementalSizeT() noexcept;
}


