#include "Timer.h"

#include <windows.h>

Timer::Timer() {
	std::int64_t countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

float Timer::TotalTime() const noexcept {
	return (float)((mCurrTime - mBaseTime) * mSecondsPerCount);
}

float Timer::DeltaTime() const noexcept {
	return (float) mDeltaTime;
}

void Timer::Reset() noexcept {
	std::int64_t currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
}

void Timer::Tick() noexcept {
	std::int64_t currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
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

