#include "NumberGeneration.h"

#include <atomic>
#include <random>

namespace NumberGeneration {
	std::size_t GetRandomSizeT() noexcept {
		static std::uniform_int_distribution<std::size_t> sUniformIntegerDistribution{ 0UL, SIZE_MAX };
		static std::mt19937 sGenerator;

		return sUniformIntegerDistribution(sGenerator);
	}

	std::size_t GetIncrementalSizeT() noexcept {
		static std::atomic<std::size_t> atomicSizeT{ 0UL };
		return atomicSizeT.fetch_add(1UL);
	}
}