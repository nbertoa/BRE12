#include "Keyboard.h"

#include <Utils/DebugUtils.h>

Keyboard* Keyboard::gInstance = nullptr;

Keyboard::Keyboard(IDirectInput8& directInput, const HWND windowHandle)
	: mDirectInput(directInput)
{
	CHECK_HR(mDirectInput.CreateDevice(GUID_SysKeyboard, &mDevice, nullptr));
	ASSERT(mDevice);
	CHECK_HR(mDevice->SetDataFormat(&c_dfDIKeyboard));
	CHECK_HR(mDevice->SetCooperativeLevel(windowHandle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));
	mDevice->Acquire();
}

Keyboard::~Keyboard() {
	mDevice->Unacquire();
	mDevice->Release();
}

void Keyboard::Update() {
	ASSERT(mDevice);
	memcpy(mLastState, mCurrentState, sizeof(mCurrentState));
	if (FAILED(mDevice->GetDeviceState(sizeof(mCurrentState), reinterpret_cast<LPVOID>(mCurrentState))) && SUCCEEDED(mDevice->Acquire())) {
		mDevice->GetDeviceState(sizeof(mCurrentState), reinterpret_cast<LPVOID>(mCurrentState));
	}
}