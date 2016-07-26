#include "Keyboard.h"

#include <memory>

#include <Utils/DebugUtils.h>

namespace {
	std::unique_ptr<Keyboard> gKeyboard{ nullptr };
}

Keyboard& Keyboard::Create(IDirectInput8& directInput, const HWND windowHandle) noexcept {
	ASSERT(gKeyboard == nullptr);
	gKeyboard.reset(new Keyboard(directInput, windowHandle));
	return *gKeyboard.get();
}

Keyboard& Keyboard::Get() noexcept {
	ASSERT(gKeyboard != nullptr);
	return *gKeyboard.get();
}

Keyboard::Keyboard(IDirectInput8& directInput, const HWND windowHandle)
	: mDirectInput(directInput)
{
	ASSERT(gKeyboard == nullptr);

	CHECK_HR(mDirectInput.CreateDevice(GUID_SysKeyboard, &mDevice, nullptr));
	ASSERT(mDevice != nullptr);
	CHECK_HR(mDevice->SetDataFormat(&c_dfDIKeyboard));
	CHECK_HR(mDevice->SetCooperativeLevel(windowHandle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));
	mDevice->Acquire();
}

Keyboard::~Keyboard() {
	mDevice->Unacquire();
	mDevice->Release();
}

void Keyboard::Update() noexcept {
	ASSERT(mDevice != nullptr);

	memcpy(mLastState, mCurrentState, sizeof(mCurrentState));
	if (FAILED(mDevice->GetDeviceState(sizeof(mCurrentState), reinterpret_cast<LPVOID>(mCurrentState))) && SUCCEEDED(mDevice->Acquire())) {
		mDevice->GetDeviceState(sizeof(mCurrentState), reinterpret_cast<LPVOID>(mCurrentState));
	}
}