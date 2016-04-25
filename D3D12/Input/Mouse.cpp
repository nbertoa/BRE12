#include "Mouse.h"

#include <Utils/DebugUtils.h>

Mouse* Mouse::gInstance = nullptr;

Mouse::Mouse(IDirectInput8& directInput, const HWND windowHandle)
	: mDirectInput(directInput)
{
	CHECK_HR(mDirectInput.CreateDevice(GUID_SysMouse, &mDevice, nullptr));
	ASSERT(mDevice);
	CHECK_HR(mDevice->SetDataFormat(&c_dfDIMouse));
	CHECK_HR(mDevice->SetCooperativeLevel(windowHandle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));
	mDevice->Acquire();
}

Mouse::~Mouse() {
	mDevice->Unacquire();
	mDevice->Release();
}

void Mouse::Update() {
	ASSERT(mDevice);
	memcpy(&mLastState, &mCurrentState, sizeof(mCurrentState));
	if (FAILED(mDevice->GetDeviceState(sizeof(mCurrentState), &mCurrentState)) && SUCCEEDED(mDevice->Acquire()) && FAILED(mDevice->GetDeviceState(sizeof(mCurrentState), &mCurrentState))) {
		return;
	}
	mX += mCurrentState.lX;
	mY += mCurrentState.lY;
	mWheel += mCurrentState.lZ;
}