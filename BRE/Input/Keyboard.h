#pragma once

#include <cstdint>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

namespace BRE {
///
/// @brief Responsible to handle keyboard updates and events
///
class Keyboard {
public:
    ///
    /// @brief Create keyboard
    ///
    /// Must be called once
    ///
    /// @param directInput Direct Input
    /// @param windowHandle Window handle
    /// @return The created keyboard
    ///
    static Keyboard& Create(IDirectInput8& directInput,
                            const HWND windowHandle) noexcept;

    ///
    /// @brief Get keyboard
    ///
    /// Create must be called first
    ///
    /// @return The keyboard
    ///
    static Keyboard& Get() noexcept;

    ~Keyboard();
    Keyboard(const Keyboard&) = delete;
    const Keyboard& operator=(const Keyboard&) = delete;
    Keyboard(Keyboard&&) = delete;
    Keyboard& operator=(Keyboard&&) = delete;

    ///
    /// @brief Update keys state
    ///
    void UpdateKeysState() noexcept;

    ///
    /// @brief Get keys current state
    ///
    /// @return Pointer to the first key in the array of keys
    ///
    __forceinline const std::uint8_t* GetKeysCurrentState() const noexcept
    {
        return mKeysCurrentState;
    }

    ///
    /// @brief Get keys last state
    ///
    /// @return Pointer to the first key in the array of keys
    ///
    __forceinline const std::uint8_t* GetKeysLastState() const noexcept
    {
        return mKeysLastState;
    }

    ///
    /// @brief Checks if key is up
    /// @param key Key to check
    /// @return True if key is up. False, otherwise
    ///
    __forceinline bool IsKeyUp(const std::uint8_t key) const noexcept
    {
        return (mKeysCurrentState[key] & 0x80) == 0U;
    }

    ///
    /// @brief Checks if key is down
    /// @param key Key to check
    /// @return True if key is down. False, otherwise
    ///
    __forceinline bool IsKeyDown(const std::uint8_t key) const noexcept
    {
        return (mKeysCurrentState[key] & 0x80) != 0U;
    }

    ///
    /// @brief Checks if key was up
    /// @param key Key to check
    /// @return True if key was up. False, otherwise
    ///
    __forceinline bool WasKeyUp(const std::uint8_t key) const noexcept
    {
        return (mKeysLastState[key] & 0x80) == 0U;
    }

    ///
    /// @brief Checks if key was down
    /// @param key Key to check
    /// @return True if key was down. False, otherwise
    ///
    __forceinline bool WasKeyDown(const std::uint8_t key) const noexcept
    {
        return (mKeysLastState[key] & 0x80) != 0U;
    }

    ///
    /// @brief Checks if key was pressed this frame
    /// @param key Key to check
    /// @return True if key was pressed this frame. False, otherwise
    ///
    __forceinline bool WasKeyPressedThisFrame(const std::uint8_t key) const noexcept
    {
        return IsKeyDown(key) && WasKeyUp(key);
    }

    ///
    /// @brief Checks if key was released this frame
    /// @param key Key to check
    /// @return True if key was released this frame. False, otherwise
    ///
    __forceinline bool WasKeyReleasedThisFrame(const std::uint8_t key) const noexcept
    {
        return IsKeyUp(key) && WasKeyDown(key);
    }

    ///
    /// @brief Checks if key is held down
    /// @param key Key to check
    /// @return True if key is held down. False, otherwise
    ///
    __forceinline bool IsKeyHeldDown(const std::uint8_t key) const noexcept
    {
        return IsKeyDown(key) && WasKeyDown(key);
    }

private:
    explicit Keyboard(IDirectInput8& directInput, const HWND windowHandle);

    static const uint32_t sNumKeys = 256U;

    IDirectInput8& mDirectInput;
    LPDIRECTINPUTDEVICE8 mDevice{ nullptr };
    std::uint8_t mKeysCurrentState[sNumKeys] = {};
    std::uint8_t mKeysLastState[sNumKeys] = {};
};
}

