#pragma once

#include <DirectXMath.h>

#include <MathUtils\MathUtils.h>

namespace BRE {
///
/// @brief Perspective Projection Camera
///
class Camera {
public:
    Camera() = default;
    ~Camera() = default;
    Camera(const Camera&) = delete;
    const Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

    ///
    /// @brief Get the camera position
    /// @return 4 homogeneous coordinates of the position of the camera
    ///
    __forceinline DirectX::XMFLOAT4 GetPosition4f() const noexcept
    {
        return DirectX::XMFLOAT4(mPosition.x, mPosition.y, mPosition.z, 1.0f);
    }

    ///
    /// @brief Set camera frustum
    /// @param verticalFieldOfView Vertical field of view angle in radians
    /// @param aspectRatio Aspect ratio of the screen
    /// @param nearPlaneZ Z coordinate of the near plane
    /// @param farPlaneZ Z coordinate of the far plane
    ///
    void SetFrustum(const float verticalFieldOfView,
                    const float aspectRatio,
                    const float nearPlaneZ,
                    const float farPlaneZ) noexcept;

    ///
    /// @brief Set look at and up camera vectors
    /// @param cameraPosition 3D coordinates of the camera position
    /// @param targetPosition 3D coordinates of the target position
    /// @param upVector 3D coordinates of the up vector
    ///
    void SetLookAndUpVectors(const DirectX::XMFLOAT3& cameraPosition,
                             const DirectX::XMFLOAT3& targetPosition,
                             const DirectX::XMFLOAT3& upVector) noexcept;

    ///
    /// @brief Get the view matrix
    /// @return View matrix as a XMFLOAT4X4
    ///
    __forceinline const DirectX::XMFLOAT4X4& GetViewMatrix() const noexcept
    {
        return mViewMatrix;
    }

    ///
    /// @brief Get the inverse of the view matrix
    /// @return Inverse of the view matrix as a XMFLOAT4X4
    ///
    __forceinline const DirectX::XMFLOAT4X4& GetInverseViewMatrix() const noexcept
    {
        return mInverseViewMatrix;
    }
    
    ///
    /// @brief Get the projection matrix
    /// @return Projection matrix as a XMFLOAT4X4
    ///
    __forceinline const DirectX::XMFLOAT4X4& GetProjectionMatrix() const noexcept
    {
        return mProjectionMatrix;
    }

    ///
    /// @brief Get the inverse of the projection matrix
    /// @return Inverse of the projection matrix as a XMFLOAT4X4
    ///
    __forceinline const DirectX::XMFLOAT4X4& GetInverseProjectionMatrix() const noexcept
    {
        return mInverseProjectionMatrix;
    }

    // If distance is positive, then we
    // will strafe left / walk forward.
    // Otherwise, we will strafe right / walk backward.
    ///
    /// @brief Camera strafe
    /// 
    /// If distance is positive, then we will strafe left. 
    /// Otherwise, we will strafe right.
    ///
    /// @param distance The distance to strafe
    ///
    void Strafe(const float distance) noexcept;

    ///
    /// @brief Camera walk
    ///
    /// If distance is positive, then we will walk forward.
    /// Otherwise, we will walk backward.
    ///
    /// @param distance The distance to walk
    ///
    void Walk(const float distance) noexcept;

    ///
    /// @brief Pitch camera
    /// @param angleInRadians Angle to pitch in radians
    ///
    void Pitch(const float angleInRadians) noexcept;

    ///
    /// @brief Rotate camera around Y axis.
    /// @param angleInRadians Angle to rotate in radians
    ///
    void RotateY(const float angleInRadians) noexcept;

    ///
    /// @brief Update camera view matrix
    ///
    /// This update depends on events like Pitch, 
    /// RotateY, Strafe, and Walk.
    ///
    void UpdateViewMatrix() noexcept;

private:
    DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 mRightVector = { 1.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 mUpVector = { 0.0f, 1.0f, 0.0f };
    DirectX::XMFLOAT3 mLookVector = { 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT3 mVelocityVector{ 0.0f, 0.0f, 0.0f };

    DirectX::XMFLOAT4X4 mViewMatrix{ MathUtils::GetIdentity4x4Matrix() };
    DirectX::XMFLOAT4X4 mInverseViewMatrix{ MathUtils::GetIdentity4x4Matrix() };
    DirectX::XMFLOAT4X4 mProjectionMatrix{ MathUtils::GetIdentity4x4Matrix() };
    DirectX::XMFLOAT4X4 mInverseProjectionMatrix{ MathUtils::GetIdentity4x4Matrix() };
};
}
 