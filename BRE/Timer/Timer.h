#pragma once

#include <cstdint>

class Timer {
public:
	Timer();
	Timer(const Timer&) = delete;
	const Timer& operator=(const Timer&) = delete;

	// Time in seconds (1.0f is 1 second)
	float TotalTime() const noexcept { return static_cast<float>((mCurrTime - mBaseTime) * mSecondsPerCount); }
	float DeltaTime() const noexcept { return static_cast<float>(mDeltaTime); }

	// Call before message loop.
	void Reset() noexcept; 

	// Call every frame.
	void Tick() noexcept;  

private:
	double mSecondsPerCount{ 0.0 };
	double mDeltaTime{ 0.0 };
	std::int64_t mBaseTime{ 0 };
	std::int64_t mPrevTime{ 0 };
	std::int64_t mCurrTime{ 0 };
};