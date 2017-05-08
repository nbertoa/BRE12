#pragma once

#include <cstdint>

namespace BRE {
class Timer {
public:
    Timer();
    ~Timer() = default;
    Timer(const Timer&) = delete;
    const Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    float TotalTimeInSeconds() const noexcept
    {
        return static_cast<float>((mCurrentTime - mBaseTime) * mSecondsPerCount);
    }
    float DeltaTimeInSeconds() const noexcept
    {
        return static_cast<float>(mDeltaTimeInSeconds);
    }

    // Call before message loop.
    void Reset() noexcept;

    // Call every frame.
    void Tick() noexcept;

private:
    double mSecondsPerCount{ 0.0 };
    double mDeltaTimeInSeconds{ 0.0 };
    std::int64_t mBaseTime{ 0 };
    std::int64_t mPreviousTime{ 0 };
    std::int64_t mCurrentTime{ 0 };
};
}

