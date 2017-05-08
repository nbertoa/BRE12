#include "Mouse.h"

#include <memory>

#include <Utils/DebugUtils.h>

namespace {
	std::unique_ptr<Mouse> gMouse{ nullptr };
}

Mouse& 
Mouse::Create(IDirectInput8& directInput, const HWND windowHandle) noexcept {
	ASSERT(gMouse == nullptr);
	gMouse.reset(new Mouse(directInput, windowHandle));
	return *gMouse.get();
}
Mouse& 
Mouse::Get() noexcept {
	ASSERT(gMouse != nullptr);
	return *gMouse.get();
}

Mouse::Mouse(IDirectInput8& directInput, const HWND windowHandle)
	: mDirectInput(directInput)
{
	ASSERT(gMouse == nullptr);

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

void 
Mouse::Update() {
	ASSERT(mDevice != nullptr);

	memcpy(&mLastState, &mCurrentState, sizeof(mCurrentState));
	if (FAILED(mDevice->GetDeviceState(sizeof(mCurrentState), &mCurrentState)) && 
		SUCCEEDED(mDevice->Acquire()) && 
		FAILED(mDevice->GetDeviceState(sizeof(mCurrentState), &mCurrentState))) {
		return;
	}

	mX += mCurrentState.lX;
	mY += mCurrentState.lY;
	mWheel += mCurrentState.lZ;
}