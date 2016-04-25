#pragma once

#include <cstdint>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class Mouse {
public:
	static Mouse* gInstance;

	const Mouse& operator=(const Mouse& rhs) = delete;

	enum MouseButton {
		MouseButtonsLeft = 0U,
		MouseButtonsRight,
		MouseButtonsMiddle,
		MouseButtonsX1
	};

	Mouse(IDirectInput8& directInput, const HWND windowHandle);
	~Mouse();

	void Update();

	const DIMOUSESTATE& CurrentState() { return mCurrentState; }
	const DIMOUSESTATE& LastState() { return mLastState; }
	uint32_t X() const { return mX; }
	uint32_t Y() const { return mY; }
	uint32_t Wheel() const { return mWheel; }
	bool IsButtonUp(const MouseButton b) const { return (mCurrentState.rgbButtons[b] & 0x80) == 0U; }
	bool IsButtonDown(const MouseButton b) const { return (mCurrentState.rgbButtons[b] & 0x80) != 0U; }
	bool WasButtonUp(const MouseButton b) const { return (mLastState.rgbButtons[b] & 0x80) == 0U; }
	bool WasButtonDown(const MouseButton b) const { return (mLastState.rgbButtons[b] & 0x80) != 0U; }
	bool WasButtonPressedThisFrame(const MouseButton b) const { return IsButtonDown(b) && WasButtonUp(b); }
	bool WasButtonReleasedThisFrame(const MouseButton b) const { return IsButtonUp(b) && WasButtonDown(b); }
	bool IsButtonHeldDown(const MouseButton b) const { return IsButtonDown(b) && WasButtonDown(b); }

private:
	IDirectInput8& mDirectInput;
	LPDIRECTINPUTDEVICE8 mDevice = nullptr;
	DIMOUSESTATE mCurrentState = {};
	DIMOUSESTATE mLastState = {};
	uint32_t mX = 0U;
	uint32_t mY = 0U;
	uint32_t mWheel = 0U;
};