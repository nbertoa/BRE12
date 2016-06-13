#include "NumberGeneration.h"

#include <atomic>
#include <random>

namespace NumberGeneration {
	std::size_t SizeTRand() noexcept {
		static std::uniform_int_distribution<std::size_t> sDistribution{ 0UL, SIZE_MAX };
		static std::mt19937 sGenerator;

		return sDistribution(sGenerator);
	}

	std::size_t IncrementalSizeT() noexcept {
		static std::atomic<std::size_t> n{ 0UL };
		return n++;
	}
}