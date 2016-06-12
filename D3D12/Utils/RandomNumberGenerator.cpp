#include "RandomNumberGenerator.h"

std::size_t sizeTRand() noexcept {
	static std::hash<std::thread::id> sHasher;
	static std::uniform_int_distribution<std::size_t> sDistribution{ 0UL, SIZE_MAX };
	static std::mt19937 sGenerator;	
	
	return sDistribution(sGenerator);
}