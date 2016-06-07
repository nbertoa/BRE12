#pragma once

#include <cstdint>
#include <random>

class RandomNumberGenerator {
public:
	RandomNumberGenerator()
		: mGenerator(mRd())
		, mDistribution(0UL, SIZE_MAX)
	{

	}
	RandomNumberGenerator(const RandomNumberGenerator&) = delete;
	const RandomNumberGenerator& operator=(const RandomNumberGenerator&) = delete;

	std::size_t RandomNumber() noexcept { return mDistribution(mGenerator); }

private:
	std::random_device mRd;
	std::mt19937 mGenerator;
	std::uniform_int_distribution<std::size_t> mDistribution;
};
