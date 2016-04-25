#pragma once

#include <DirectXMath.h>

#include <MathUtils\MathHelper.h>
#include <Utils\DebugUtils.h>

class Camera {
public:
	Camera();

	// Get/Set world camera position.
	DirectX::XMVECTOR GetPosition() const { return XMLoadFloat3(&mPosition); }
	DirectX::XMFLOAT3 GetPosition3f() const { return mPosition; }
	void SetPosition(const float x, const float y, const float z) {
		mPosition = DirectX::XMFLOAT3(x, y, z);
		mViewDirty = true;
	}
	void SetPosition(const DirectX::XMFLOAT3& v) {
		mPosition = v;
		mViewDirty = true;
	}
	
	// Get camera basis vectors.
	DirectX::XMVECTOR GetRight() const { return XMLoadFloat3(&mRight); }
	DirectX::XMFLOAT3 GetRight3f() const { return mRight; }
	DirectX::XMVECTOR GetUp() const { return XMLoadFloat3(&mUp); }
	DirectX::XMFLOAT3 GetUp3f() const { return mUp; }
	DirectX::XMVECTOR GetLook() const { return XMLoadFloat3(&mLook); }
	DirectX::XMFLOAT3 GetLook3f() const { return mLook; }

	// Get frustum properties.
	float GetNearZ() const { return mNearZ; }
	float GetFarZ() const { return mFarZ; }
	float GetAspect() const { return mAspect; }
	float GetFovY() const { return mFovY; }
	float GetFovX() const {
		const float halfWidth = 0.5f * GetNearWindowWidth();
		return (float) (2.0 * atan(halfWidth / mNearZ));
	}

	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth() const { return mAspect * mNearWindowHeight; }
	float GetNearWindowHeight() const { return mNearWindowHeight; }
	float GetFarWindowWidth() const { return mAspect * mFarWindowHeight; }
	float GetFarWindowHeight() const { return mFarWindowHeight; }
	
	// Set frustum.
	void SetLens(const float fovY, const float aspect, const float zn, const float zf);

	// Define camera space via LookAt parameters.
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

	// Get View/Proj matrices.
	DirectX::XMMATRIX GetView() const {
		ASSERT(!mViewDirty);
		return XMLoadFloat4x4(&mView);
	}
	DirectX::XMMATRIX GetProj() const { return XMLoadFloat4x4(&mProj); }

	DirectX::XMFLOAT4X4 GetView4x4f() const {
		ASSERT(!mViewDirty);
		return mView;
	}
	DirectX::XMFLOAT4X4 GetProj4x4f() const { return mProj; }

	// Strafe/Walk the camera a distance d.
	void Strafe(const float d);
	void Walk(const float d);

	// Rotate the camera.
	void Pitch(const float angle);
	void RotateY(const float angle);

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();

private:
	// Camera coordinate system with coordinates relative to world space.
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	// Cache frustum properties.
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	// Cache View/Proj matrices.
	DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();
};