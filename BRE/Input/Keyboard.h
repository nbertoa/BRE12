#pragma once

#include <cstdint>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

namespace BRE {
class Keyboard {
public:
    // Preconditions:
    // - Create() must be called once
    static Keyboard& Create(IDirectInput8& directInput,
                            const HWND windowHandle) noexcept;

    // Preconditions:
    // - Create() method must be called before.
    static Keyboard& Get() noexcept;

    ~Keyboard();
    Keyboard(const Keyboard&) = delete;
    const Keyboard& operator=(const Keyboard&) = delete;
    Keyboard(Keyboard&&) = delete;
    Keyboard& operator=(Keyboard&&) = delete;

    void Update() noexcept;

    __forceinline const std::uint8_t* GetKeysCurrentState() const noexcept
    {
        return mKeysCurrentState;
    }
    __forceinline const std::uint8_t* GetKeysLastState() const noexcept
    {
        return mKeysLastState;
    }
    __forceinline bool IsKeyUp(const std::uint8_t key) const noexcept
    {
        return (mKeysCurrentState[key] & 0x80) == 0U;
    }
    __forceinline bool IsKeyDown(const std::uint8_t key) const noexcept
    {
        return (mKeysCurrentState[key] & 0x80) != 0U;
    }
    __forceinline bool WasKeyUp(const std::uint8_t key) const noexcept
    {
        return (mKeysLastState[key] & 0x80) == 0U;
    }
    __forceinline bool WasKeyDown(const std::uint8_t key) const noexcept
    {
        return (mKeysLastState[key] & 0x80) != 0U;
    }
    __forceinline bool WasKeyPressedThisFrame(const std::uint8_t key) const noexcept
    {
        return IsKeyDown(key) && WasKeyUp(key);
    }
    __forceinline bool WasKeyReleasedThisFrame(const std::uint8_t key) const noexcept
    {
        return IsKeyUp(key) && WasKeyDown(key);
    }
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

