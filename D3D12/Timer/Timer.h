#pragma once

#include <cstdint>

class Timer {
public:
	Timer();
	Timer(const Timer&) = delete;
	const Timer& operator=(const Timer&) = delete;

	float TotalTime() const noexcept; // in seconds
	float DeltaTime() const noexcept; // in seconds

	void Reset() noexcept; // Call before message loop.
	void Start() noexcept; // Call when unpaused.
	void Stop() noexcept;  // Call when paused.
	void Tick() noexcept;  // Call every frame.

private:
	double mSecondsPerCount{ 0.0 };
	double mDeltaTime{ 0.0 };

	std::int64_t mBaseTime{ 0 };
	std::int64_t mPausedTime{ 0 };
	std::int64_t mStopTime{ 0 };
	std::int64_t mPrevTime{ 0 };
	std::int64_t mCurrTime{ 0 };

	bool mStopped{ false };
};