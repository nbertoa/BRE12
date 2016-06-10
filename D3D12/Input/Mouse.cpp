#include "Mouse.h"

#include <Utils/DebugUtils.h>

std::unique_ptr<Mouse> Mouse::gMouse{ nullptr };

Mouse::Mouse(IDirectInput8& directInput, const HWND windowHandle)
	: mDirectInput(directInput)
{
	CHECK_HR(mDirectInput.CreateDevice(GUID_SysMouse, &mDevice, nullptr));
	ASSERT(mDevice != nullptr);
	CHECK_HR(mDevice->SetDataFormat(&c_dfDIMouse));
	CHECK_HR(mDevice->SetCooperativeLevel(windowHandle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));
	mDevice->Acquire();
}

Mouse::~Mouse() {
	ASSERT(mDevice != nullptr);

	mDevice->Unacquire();
	mDevice->Release();
}

void Mouse::Update() {
	ASSERT(mDevice != nullptr);

	memcpy(&mLastState, &mCurrentState, sizeof(mCurrentState));
	if (FAILED(mDevice->GetDeviceState(sizeof(mCurrentState), &mCurrentState)) && SUCCEEDED(mDevice->Acquire()) && FAILED(mDevice->GetDeviceState(sizeof(mCurrentState), &mCurrentState))) {
		return;
	}

	mX += mCurrentState.lX;
	mY += mCurrentState.lY;
	mWheel += mCurrentState.lZ;
}