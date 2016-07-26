#include "Camera.h"

using namespace DirectX;

void Camera::SetLens(const float fovY, const float aspect, const float zn, const float zf) noexcept {
	// cache properties
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	mNearWindowHeight = 2.0f * mNearZ * tanf( 0.5f * mFovY );
	mFarWindowHeight  = 2.0f * mFarZ * tanf( 0.5f * mFovY );

	const XMMATRIX P{ XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ) };
	XMStoreFloat4x4(&mProj, P);

	mViewDirty = true;
}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp) noexcept {
	const XMVECTOR L( XMVector3Normalize(XMVectorSubtract(target, pos)) );
	const XMVECTOR R( XMVector3Normalize(XMVector3Cross(worldUp, L)) );
	const XMVECTOR U( XMVector3Cross(L, R) );

	XMStoreFloat3(&mPosition, pos);
	XMStoreFloat3(&mLook, L);
	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);

	mViewDirty = true;
}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up) noexcept {
	const XMVECTOR P( XMLoadFloat3(&pos) );
	const XMVECTOR T( XMLoadFloat3(&target) );
	const XMVECTOR U( XMLoadFloat3(&up) );

	LookAt(P, T, U);

	mViewDirty = true;
}

void Camera::GetView4x4f(DirectX::XMFLOAT4X4& m) const noexcept {
	m = mView;
}

void Camera::GetProj4x4f(DirectX::XMFLOAT4X4& m) const noexcept {
	m = mProj;
}

void Camera::Strafe(const float d) noexcept {
	// mPosition += d * mRight
	const XMVECTOR s(XMVectorReplicate(d));
	const XMVECTOR r(XMLoadFloat3(&mRight));
	const XMVECTOR p(XMLoadFloat3(&mPosition));
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, r, p));

	mViewDirty = true;
}

void Camera::Walk(const float d) noexcept {
	// mPosition += d * mLook
	const XMVECTOR s(XMVectorReplicate(d));
	const XMVECTOR l(XMLoadFloat3(&mLook));
	const XMVECTOR p(XMLoadFloat3(&mPosition));
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, l, p));

	mViewDirty = true;
}

void Camera::Pitch(const float angle) noexcept {
	// Rotate up and look vector about the right vector.
	const XMMATRIX R(XMMatrixRotationAxis(XMLoadFloat3(&mRight), angle));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

void Camera::RotateY(const float angle) noexcept {
	// Rotate the basis vectors about the world y-axis.
	const XMMATRIX R(XMMatrixRotationY(angle));
	XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), R));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));
	
	mViewDirty = true;
}

bool Camera::UpdateViewMatrix() noexcept {
	if(mViewDirty) {
		XMVECTOR R( XMLoadFloat3(&mRight) );
		XMVECTOR U( XMLoadFloat3(&mUp) );
		XMVECTOR L( XMLoadFloat3(&mLook) );
		const XMVECTOR P( XMLoadFloat3(&mPosition) );

		// Keep camera's axes orthogonal to each other and of unit length.
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));

		// U, L already orthonormal, so no need to normalize cross product.
		R = XMVector3Cross(U, L);

		XMStoreFloat3(&mRight, R);
		XMStoreFloat3(&mUp, U);
		XMStoreFloat3(&mLook, L);

		XMMATRIX viewMatrix = XMMatrixLookToLH(P, L, U);
		XMStoreFloat4x4(&mView, viewMatrix);

		mViewDirty = false;
		return true;
	}

	return false;
}