#include "Timer.h"

#include <windows.h>

namespace BRE {
Timer::Timer()
{
    std::int64_t countsPerSecond;
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSecond));
    mSecondsPerCount = 1.0 / static_cast<double>(countsPerSecond);
    Reset();
}

void
Timer::Reset() noexcept
{
    std::int64_t currentTime;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentTime));

    mStartTickTime = currentTime;
    mPreviousTickTime = currentTime;
}

void
Timer::Tick() noexcept
{
    std::int64_t currentTime;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentTime));

    mDeltaTimeInSeconds = (currentTime - mPreviousTickTime) * mSecondsPerCount;

    mPreviousTickTime = currentTime;

    // Force nonnegative. The DXSDK's CDXUTTimer mentions that if the 
    // processor goes into a power save mode or we get shuffled to another
    // processor, then mGetDeltaTimeInSeconds can be negative.
    if (mDeltaTimeInSeconds < 0.0) {
        mDeltaTimeInSeconds = 0.0;
    }
}
}