#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <cstdint>
#include <dinput.h>

namespace BRE {
///
/// @brief Responsible to handle mouse updates and events
///
class Mouse {
public:
    ///
    /// @brief Create mouse
    ///
    /// Must be called once
    ///
    /// @param directInput Direct Input
    /// @param windowHandle Window handle
    /// @return The created mouse
    ///
    static Mouse& Create(IDirectInput8& directInput,
                         const HWND windowHandle) noexcept;

    ///
    /// @brief Get mouse
    ///
    /// Create must be called first
    ///
    /// @return The mouse
    ///
    static Mouse& Get() noexcept;

    enum MouseButton {
        MouseButtonsLeft = 0,
        MouseButtonsRight,
        MouseButtonsMiddle,
        MouseButtonsX1
    };

    ~Mouse();
    Mouse(const Mouse&) = delete;
    const Mouse& operator=(const Mouse&) = delete;
    Mouse(Mouse&&) = delete;
    Mouse& operator=(Mouse&&) = delete;

    ///
    /// @brief Update mouse state
    ///
    void UpdateMouseState();

    ///
    /// @brief Get mouse current state
    ///
    /// @return Mouse state
    ///
    __forceinline const DIMOUSESTATE& GetCurrentState() const
    {
        return mCurrentState;
    }

    ///
    /// @brief Get mouse last state
    ///
    /// @return Mouse state
    ///
    __forceinline const DIMOUSESTATE& GetLastState() const
    {
        return mLastState;
    }

    ///
    /// @brief Get x mouse coordinate
    /// @return Coordinate
    ///
    __forceinline std::int32_t GetX() const
    {
        return mX;
    }

    ///
    /// @brief Get y mouse coordinate
    /// @return Coordinate
    ///
    __forceinline std::int32_t GetY() const
    {
        return mY;
    }

    ///
    /// @brief Get wheel value
    /// @return Wheel value
    ///
    __forceinline std::int32_t GetWheel() const
    {
        return mWheel;
    }

    ///
    /// @brief Checks if button is up
    /// @param button Button to check
    /// @return True if button is up. False, otherwise
    ///
    __forceinline bool IsButtonUp(const MouseButton button) const
    {
        return (mCurrentState.rgbButtons[button] & 0x80) == 0;
    }

    ///
    /// @brief Checks if button is down
    /// @param button Button to check
    /// @return True if button is down. False, otherwise
    ///
    __forceinline bool IsButtonDown(const MouseButton button) const
    {
        return (mCurrentState.rgbButtons[button] & 0x80) != 0;
    }

    ///
    /// @brief Checks if button was up
    /// @param button Button to check
    /// @return True if button was up. False, otherwise
    ///
    __forceinline bool WasButtonUp(const MouseButton button) const
    {
        return (mLastState.rgbButtons[button] & 0x80) == 0;
    }

    ///
    /// @brief Checks if button was down
    /// @param button Button to check
    /// @return True if button was down. False, otherwise
    ///
    __forceinline bool WasButtonDown(const MouseButton button) const
    {
        return (mLastState.rgbButtons[button] & 0x80) != 0;
    }

    ///
    /// @brief Checks if button was pressed this frame
    /// @param button Button to check
    /// @return True if button was pressed this frame. False, otherwise
    ///
    __forceinline bool WasButtonPressedThisFrame(const MouseButton button) const
    {
        return IsButtonDown(button) && WasButtonUp(button);
    }

    ///
    /// @brief Checks if button was released this frame
    /// @param button Button to check
    /// @return True if button was released this frame. False, otherwise
    ///
    __forceinline bool WasButtonReleasedThisFrame(const MouseButton button) const
    {
        return IsButtonUp(button) && WasButtonDown(button);
    }

    ///
    /// @brief Checks if button is held down
    /// @param button Button to check
    /// @return True if button is held down. False, otherwise
    ///
    __forceinline bool IsButtonHeldDown(const MouseButton button) const
    {
        return IsButtonDown(button) && WasButtonDown(button);
    }

private:
    explicit Mouse(IDirectInput8& directInput, const HWND windowHandle);

    IDirectInput8& mDirectInput;
    LPDIRECTINPUTDEVICE8 mDevice{ nullptr };
    DIMOUSESTATE mCurrentState{};
    DIMOUSESTATE mLastState{};
    std::int32_t mX{ 0 };
    std::int32_t mY{ 0 };
    std::int32_t mWheel{ 0 };
};
}

