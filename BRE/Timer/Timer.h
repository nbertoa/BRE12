#pragma once

#include <cstdint>

namespace BRE {
///
/// @brief Timer class used mainly to get frame time.
///
class Timer {
public:
    Timer();
    ~Timer() = default;
    Timer(const Timer&) = delete;
    const Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    /// @brief Get delta time in seconds.
    ///
    /// Get delta time in seconds between 2 ticks, used to know the elapsed time between frames.
    ///
    /// @return Time in seconds
    ///
    __forceinline float GetDeltaTimeInSeconds() const noexcept
    {
        return static_cast<float>(mDeltaTimeInSeconds);
    }

    /// @brief Reset timer
    ///
    ///  This should be called exactly before the game loop.
    ///
    void Reset() noexcept;

    /// @brief Ticks the timer
    ///
    /// This will cause that delta time in seconds is updated.
    /// You should call it every frame.
    ///
    void Tick() noexcept;

private:
    double mSecondsPerCount{ 0.0 };
    double mDeltaTimeInSeconds{ 0.0 };
    std::int64_t mStartTickTime{ 0 };
    std::int64_t mPreviousTickTime{ 0 };
};
}

