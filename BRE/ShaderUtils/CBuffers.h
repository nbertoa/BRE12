#pragma once

#include <DirectXMath.h>

#include <MathUtils\MathUtils.h>
#include <ApplicationSettings\ApplicationSettings.h>

namespace BRE {
///
/// @brief Constant buffer per object
///
struct ObjectCBuffer {
    ObjectCBuffer() = default;
    ~ObjectCBuffer() = default;
    ObjectCBuffer(const ObjectCBuffer&) = default;
    ObjectCBuffer(ObjectCBuffer&&) = default;
    ObjectCBuffer& operator=(ObjectCBuffer&&) = default;

    DirectX::XMFLOAT4X4 mWorldMatrix{ MathUtils::GetIdentity4x4Matrix() };
    DirectX::XMFLOAT4X4 mInverseTransposeWorldMatrix{ MathUtils::GetIdentity4x4Matrix() };
    float mTextureScaleFactor{ 2.0f };
};

///
/// @brief Constant buffer per frame
///
struct FrameCBuffer {
    FrameCBuffer() = default;
    ~FrameCBuffer() = default;
    FrameCBuffer(const FrameCBuffer&) = default;
    const FrameCBuffer& operator=(const FrameCBuffer&);
    FrameCBuffer(FrameCBuffer&&) = default;
    FrameCBuffer& operator=(FrameCBuffer&&) = default;

    DirectX::XMFLOAT4X4 mViewMatrix{ MathUtils::GetIdentity4x4Matrix() };
    DirectX::XMFLOAT4X4 mInverseViewMatrix{ MathUtils::GetIdentity4x4Matrix() };
    DirectX::XMFLOAT4X4 mProjectionMatrix{ MathUtils::GetIdentity4x4Matrix() };
    DirectX::XMFLOAT4X4 mInverseProjectionMatrix{ MathUtils::GetIdentity4x4Matrix() };
    DirectX::XMFLOAT4 mEyeWorldPosition{ 0.0f, 0.0f, 0.0f, 1.0f };
};

///
/// @brief Immutable constant buffer (does not change across frames or objects) 
///
struct ImmutableCBuffer {
    ImmutableCBuffer() = default;
    ~ImmutableCBuffer() = default;
    ImmutableCBuffer(const ImmutableCBuffer&) = default;
    ImmutableCBuffer(ImmutableCBuffer&&) = default;
    ImmutableCBuffer& operator=(ImmutableCBuffer&&) = default;

    float mNearZ_FarZ_ScreenW_ScreenH[4U]{
        ApplicationSettings::sNearPlaneZ,
        ApplicationSettings::sFarPlaneZ,
        static_cast<float>(ApplicationSettings::sWindowWidth),
        static_cast<float>(ApplicationSettings::sWindowHeight)
    };
};

}

