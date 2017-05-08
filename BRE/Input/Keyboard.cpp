#include "Keyboard.h"

#include <memory>

#include <Utils/DebugUtils.h>

namespace BRE {
namespace {
std::unique_ptr<Keyboard> gKeyboard{ nullptr };
}

Keyboard&
Keyboard::Create(IDirectInput8& directInput, const HWND windowHandle) noexcept
{
    BRE_ASSERT(gKeyboard == nullptr);
    gKeyboard.reset(new Keyboard(directInput, windowHandle));
    return *gKeyboard.get();
}

Keyboard&
Keyboard::Get() noexcept
{
    BRE_ASSERT(gKeyboard != nullptr);
    return *gKeyboard.get();
}

Keyboard::Keyboard(IDirectInput8& directInput,
                   const HWND windowHandle)
    : mDirectInput(directInput)
{
    BRE_ASSERT(gKeyboard == nullptr);

    BRE_CHECK_HR(mDirectInput.CreateDevice(GUID_SysKeyboard, &mDevice, nullptr));
    BRE_ASSERT(mDevice != nullptr);
    BRE_CHECK_HR(mDevice->SetDataFormat(&c_dfDIKeyboard));
    BRE_CHECK_HR(mDevice->SetCooperativeLevel(windowHandle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));
    mDevice->Acquire();
}

Keyboard::~Keyboard()
{
    mDevice->Unacquire();
    mDevice->Release();
}

void
Keyboard::Update() noexcept
{
    BRE_ASSERT(mDevice != nullptr);

    memcpy(mKeysLastState, mKeysCurrentState, sizeof(mKeysCurrentState));
    if (FAILED(mDevice->GetDeviceState(sizeof(mKeysCurrentState), reinterpret_cast<LPVOID>(mKeysCurrentState))) &&
        SUCCEEDED(mDevice->Acquire())) {
        mDevice->GetDeviceState(sizeof(mKeysCurrentState), reinterpret_cast<LPVOID>(mKeysCurrentState));
    }
}
}

