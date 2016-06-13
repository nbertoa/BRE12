#pragma once

#include <cstddef>

namespace NumberGeneration {
	// Random size_t 
	std::size_t SizeTRand() noexcept;

	// Thread safe method to get all size_t values from 0 to SIZE_T_MAX.
	std::size_t IncrementalSizeT() noexcept;
}


