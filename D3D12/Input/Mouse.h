#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <cstdint>
#include <dinput.h>

class Mouse {
public:
	static Mouse& Create(IDirectInput8& directInput, const HWND windowHandle) noexcept;
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

	// You should call update in each frame
	void Update();

	__forceinline const DIMOUSESTATE& CurrentState() const { return mCurrentState; }
	__forceinline const DIMOUSESTATE& LastState() const { return mLastState; }
	__forceinline std::int32_t X() const { return mX; }
	__forceinline std::int32_t Y() const { return mY; }
	__forceinline std::int32_t Wheel() const { return mWheel; }
	__forceinline bool IsButtonUp(const MouseButton b) const { return (mCurrentState.rgbButtons[b] & 0x80) == 0; }
	__forceinline bool IsButtonDown(const MouseButton b) const { return (mCurrentState.rgbButtons[b] & 0x80) != 0; }
	__forceinline bool WasButtonUp(const MouseButton b) const { return (mLastState.rgbButtons[b] & 0x80) == 0; }
	__forceinline bool WasButtonDown(const MouseButton b) const { return (mLastState.rgbButtons[b] & 0x80) != 0; }
	__forceinline bool WasButtonPressedThisFrame(const MouseButton b) const { return IsButtonDown(b) && WasButtonUp(b); }
	__forceinline bool WasButtonReleasedThisFrame(const MouseButton b) const { return IsButtonUp(b) && WasButtonDown(b); }
	__forceinline bool IsButtonHeldDown(const MouseButton b) const { return IsButtonDown(b) && WasButtonDown(b); }

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