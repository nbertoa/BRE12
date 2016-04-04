#pragma once

#include <cstdint>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class Keyboard {
public:
	static Keyboard* gInstance;

	const Keyboard& operator=(const Keyboard& rhs) = delete;

	Keyboard(IDirectInput8& directInput, const HWND windowHandle);
	~Keyboard();

	void Update();

	const uint8_t* const CurrentState() const { return mCurrentState; }
	const uint8_t* const LastState() const { return mLastState; }
	bool IsKeyUp(const uint8_t key) const { return (mCurrentState[key] & 0x80) == 0; }
	bool IsKeyDown(const uint8_t key) const { return (mCurrentState[key] & 0x80) != 0; }
	bool WasKeyUp(const uint8_t key) const { return (mLastState[key] & 0x80) == 0; }
	bool WasKeyDown(const uint8_t key) const { return (mLastState[key] & 0x80) != 0; }
	bool WasKeyPressedThisFrame(const uint8_t key) const { return IsKeyDown(key) && WasKeyUp(key); }
	bool WasKeyReleasedThisFrame(const uint8_t key) const { return IsKeyUp(key) && WasKeyDown(key); }
	bool IsKeyHeldDown(const uint8_t key) const { return IsKeyDown(key) && WasKeyDown(key); }

private:
	IDirectInput8& mDirectInput;
	LPDIRECTINPUTDEVICE8 mDevice = nullptr;
	uint8_t mCurrentState[256] = {};
	uint8_t mLastState[256] = {};
};