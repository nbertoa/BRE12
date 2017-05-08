#include "Camera.h"

#include <SettingsManager\SettingsManager.h>
#include <Utils\DebugUtils.h>

using namespace DirectX;

namespace BRE {

void
Camera::SetFrustum(const float verticalFieldOfView,
                   const float aspectRatio,
                   const float nearPlaneZ,
                   const float farPlaneZ) noexcept
{
    const XMMATRIX projectionMatrix{ XMMatrixPerspectiveFovLH(verticalFieldOfView, aspectRatio, nearPlaneZ, farPlaneZ) };
    XMStoreFloat4x4(&mProjectionMatrix, projectionMatrix);
    MathUtils::StoreInverseMatrix(mProjectionMatrix, mInverseProjectionMatrix);
}

void
Camera::SetLookAndUpVectors(const XMFLOAT3& cameraPosition,
                            const XMFLOAT3& targetPosition,
                            const XMFLOAT3& upVector) noexcept
{
    const XMVECTOR xmCameraPosition(XMLoadFloat3(&cameraPosition));
    const XMVECTOR xmTargetPosition(XMLoadFloat3(&targetPosition));
    XMVECTOR xmUpVector(XMLoadFloat3(&upVector));

    const XMVECTOR lookVector(XMVector3Normalize(XMVectorSubtract(xmTargetPosition, xmCameraPosition)));
    const XMVECTOR rightVector(XMVector3Normalize(XMVector3Cross(xmUpVector, lookVector)));
    xmUpVector = XMVector3Cross(lookVector, rightVector);

    XMStoreFloat3(&mPosition, xmCameraPosition);
    XMStoreFloat3(&mLookVector, lookVector);
    XMStoreFloat3(&mRightVector, rightVector);
    XMStoreFloat3(&mUpVector, xmUpVector);
}

void
Camera::Strafe(const float distance) noexcept
{
    // velocity += right * dist 
    XMVECTOR rightVector(XMLoadFloat3(&mRightVector));
    rightVector = XMVectorScale(rightVector, distance);
    XMVECTOR velocityVector = XMLoadFloat3(&mVelocityVector);
    velocityVector = XMVectorAdd(velocityVector, rightVector);
    XMStoreFloat3(&mVelocityVector, velocityVector);
}

void
Camera::Walk(const float distance) noexcept
{
    // velocity += look * dist 
    XMVECTOR lookVector(XMLoadFloat3(&mLookVector));
    lookVector = XMVectorScale(lookVector, distance);
    XMVECTOR velocityVector = XMLoadFloat3(&mVelocityVector);
    velocityVector = XMVectorAdd(velocityVector, lookVector);
    XMStoreFloat3(&mVelocityVector, velocityVector);
}

void
Camera::Pitch(const float angleInRadians) noexcept
{
    // Rotate up and look vector about the right vector.
    const XMMATRIX rightVector(XMMatrixRotationAxis(XMLoadFloat3(&mRightVector), angleInRadians));
    XMStoreFloat3(&mUpVector, XMVector3TransformNormal(XMLoadFloat3(&mUpVector), rightVector));
    XMStoreFloat3(&mLookVector, XMVector3TransformNormal(XMLoadFloat3(&mLookVector), rightVector));
}

void
Camera::RotateY(const float angleInRadians) noexcept
{
    // Rotate the basis vectors about the world y-axis.
    const XMMATRIX rotationYMatrix(XMMatrixRotationY(angleInRadians));
    XMStoreFloat3(&mRightVector, XMVector3TransformNormal(XMLoadFloat3(&mRightVector), rotationYMatrix));
    XMStoreFloat3(&mUpVector, XMVector3TransformNormal(XMLoadFloat3(&mUpVector), rotationYMatrix));
    XMStoreFloat3(&mLookVector, XMVector3TransformNormal(XMLoadFloat3(&mLookVector), rotationYMatrix));
}

void
Camera::UpdateViewMatrix() noexcept
{
    static float maxVelocitySpeed{ 100.0f }; // speed = velocity magnitude
    static float velocityDamp{ 0.1f }; // fraction of velocity retained per second

    const float secondsPerFrame = SettingsManager::sSecondsPerFrame;

    // Clamp velocity
    XMVECTOR velocityVector = XMLoadFloat3(&mVelocityVector);
    const float velocityLength = XMVectorGetX(XMVector3Length(velocityVector));
    const float velocitySpeed = MathUtils::Clamp(velocityLength, 0.0f, maxVelocitySpeed);
    if (velocitySpeed > 0.0f) {
        velocityVector = XMVector3Normalize(velocityVector) * velocitySpeed;
    }

    // Apply velocity and pitch-way-roll
    XMVECTOR position = XMLoadFloat3(&mPosition);
    position = position + velocityVector * secondsPerFrame;

    // Damp velocity
    velocityVector = velocityVector * static_cast<float>(pow(velocityDamp, secondsPerFrame));

    // Keep camera's axes orthogonal to each other and of unit length.
    XMVECTOR rightVector(XMLoadFloat3(&mRightVector));
    XMVECTOR lookVector(XMLoadFloat3(&mLookVector));
    lookVector = XMVector3Normalize(lookVector);
    XMVECTOR upVector = XMVector3Normalize(XMVector3Cross(lookVector, rightVector));

    // Up vector and look vector are already orthonormal, so no need to normalize cross product.
    rightVector = XMVector3Cross(upVector, lookVector);

    XMStoreFloat3(&mRightVector, rightVector);
    XMStoreFloat3(&mUpVector, upVector);
    XMStoreFloat3(&mLookVector, lookVector);
    XMStoreFloat3(&mPosition, position);
    XMStoreFloat3(&mVelocityVector, velocityVector);

    XMMATRIX viewMatrix = XMMatrixLookToLH(position, lookVector, upVector);
    XMStoreFloat4x4(&mViewMatrix, viewMatrix);
    MathUtils::StoreInverseMatrix(mViewMatrix, mInverseViewMatrix);
}
}