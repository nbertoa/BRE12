#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <cstdint>
#include <dinput.h>

class Mouse {
public:
	// Preconditions:
	// - Create() must be called once
	static Mouse& Create(IDirectInput8& directInput, const HWND windowHandle) noexcept;

	// Preconditions:
	// - Create() method must be called before
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

	void Update();

	__forceinline const DIMOUSESTATE& GetCurrentState() const { return mCurrentState; }
	__forceinline const DIMOUSESTATE& GetLastState() const { return mLastState; }
	__forceinline std::int32_t GetX() const { return mX; }
	__forceinline std::int32_t GetY() const { return mY; }
	__forceinline std::int32_t GetWheel() const { return mWheel; }
	__forceinline bool IsButtonUp(const MouseButton button) const { return (mCurrentState.rgbButtons[button] & 0x80) == 0; }
	__forceinline bool IsButtonDown(const MouseButton button) const { return (mCurrentState.rgbButtons[button] & 0x80) != 0; }
	__forceinline bool WasButtonUp(const MouseButton button) const { return (mLastState.rgbButtons[button] & 0x80) == 0; }
	__forceinline bool WasButtonDown(const MouseButton button) const { return (mLastState.rgbButtons[button] & 0x80) != 0; }
	__forceinline bool WasButtonPressedThisFrame(const MouseButton button) const { return IsButtonDown(button) && WasButtonUp(button); }
	__forceinline bool WasButtonReleasedThisFrame(const MouseButton button) const { return IsButtonUp(button) && WasButtonDown(button); }
	__forceinline bool IsButtonHeldDown(const MouseButton button) const { return IsButtonDown(button) && WasButtonDown(button); }

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