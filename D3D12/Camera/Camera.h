#pragma once

#include <DirectXMath.h>
#include <memory>

#include <MathUtils\MathHelper.h>
#include <Utils\DebugUtils.h>

class Camera {
public:
	static std::unique_ptr<Camera> gCamera;

	Camera();

	// Get/Set world camera position.
	__forceinline DirectX::XMVECTOR GetPosition() const noexcept { return XMLoadFloat3(&mPosition); }
	__forceinline DirectX::XMFLOAT3 GetPosition3f() const noexcept { return mPosition; }
	__forceinline void SetPosition(const float x, const float y, const float z) noexcept {
		mPosition = DirectX::XMFLOAT3(x, y, z);
		mViewDirty = true;
	}
	__forceinline void SetPosition(const DirectX::XMFLOAT3& v) noexcept {
		mPosition = v;
		mViewDirty = true;
	}
	
	// Get camera basis vectors.
	__forceinline DirectX::XMVECTOR GetRight() const noexcept { return XMLoadFloat3(&mRight); }
	__forceinline DirectX::XMFLOAT3 GetRight3f() const noexcept { return mRight; }
	__forceinline DirectX::XMVECTOR GetUp() const noexcept { return XMLoadFloat3(&mUp); }
	__forceinline DirectX::XMFLOAT3 GetUp3f() const noexcept { return mUp; }
	__forceinline DirectX::XMVECTOR GetLook() const noexcept { return XMLoadFloat3(&mLook); }
	__forceinline DirectX::XMFLOAT3 GetLook3f() const noexcept { return mLook; }

	// Get frustum properties.
	__forceinline float GetNearZ() const noexcept { return mNearZ; }
	__forceinline float GetFarZ() const noexcept { return mFarZ; }
	__forceinline float GetAspect() const noexcept { return mAspect; }
	__forceinline float GetFovY() const noexcept { return mFovY; }
	__forceinline float GetFovX() const noexcept {
		const float halfWidth{ 0.5f * GetNearWindowWidth() };
		return (float) (2.0 * atan(halfWidth / mNearZ));
	}

	// Get near and far plane dimensions in view space coordinates.
	__forceinline float GetNearWindowWidth() const noexcept { return mAspect * mNearWindowHeight; }
	__forceinline float GetNearWindowHeight() const noexcept { return mNearWindowHeight; }
	__forceinline float GetFarWindowWidth() const noexcept { return mAspect * mFarWindowHeight; }
	__forceinline float GetFarWindowHeight() const noexcept { return mFarWindowHeight; }
	
	// Set frustum.
	void SetLens(const float fovY, const float aspect, const float zn, const float zf) noexcept;

	// Define camera space via LookAt parameters.
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp) noexcept;
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up) noexcept;

	// Get View/Proj matrices.
	__forceinline DirectX::XMMATRIX GetView() const noexcept {
		ASSERT(!mViewDirty);
		return XMLoadFloat4x4(&mView);
	}
	__forceinline DirectX::XMMATRIX GetProj() const noexcept { return XMLoadFloat4x4(&mProj); }

	__forceinline DirectX::XMMATRIX GetViewProj() const noexcept {
		const DirectX::XMMATRIX viewMatrix = GetView();
		const DirectX::XMMATRIX projMatrix = GetProj();
		return DirectX::XMMatrixMultiply(viewMatrix, projMatrix);
	}

	__forceinline DirectX::XMFLOAT4X4 GetView4x4f() const noexcept {
		ASSERT(!mViewDirty);
		return mView;
	}
	__forceinline DirectX::XMFLOAT4X4 GetProj4x4f() const noexcept { return mProj; }

	// Strafe/Walk the camera a distance d.
	void Strafe(const float d) noexcept;
	void Walk(const float d) noexcept;

	// Rotate the camera.
	void Pitch(const float angle) noexcept;
	void RotateY(const float angle) noexcept;

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix() noexcept;

private:
	// Camera coordinate system with coordinates relative to world space.
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	// Cache frustum properties.
	float mNearZ{ 0.0f };
	float mFarZ{ 0.0f };
	float mAspect{ 0.0f };
	float mFovY{ 0.0f };
	float mNearWindowHeight{ 0.0f };
	float mFarWindowHeight{ 0.0f };

	bool mViewDirty{ true };

	// Cache View/Proj matrices.
	DirectX::XMFLOAT4X4 mView{ MathHelper::Identity4x4() };
	DirectX::XMFLOAT4X4 mProj{ MathHelper::Identity4x4() };
};