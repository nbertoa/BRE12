#include "Timer.h"

#include <windows.h>

Timer::Timer() {
	std::int64_t countsPerSec;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSec));
	mSecondsPerCount = 1.0 / (double)countsPerSec;
	Reset();
}

void Timer::Reset() noexcept {
	std::int64_t currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

	mBaseTime = currTime;
	mPrevTime = currTime;
}

void Timer::Tick() noexcept {
	std::int64_t currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));
	mCurrTime = currTime;

	// Time difference between this frame and the previous.
	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

	// Prepare for next frame.
	mPrevTime = mCurrTime;

	// Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the 
	// processor goes into a power save mode or we get shuffled to another
	// processor, then mDeltaTime can be negative.
	if(mDeltaTime < 0.0) {
		mDeltaTime = 0.0;
	}
}

