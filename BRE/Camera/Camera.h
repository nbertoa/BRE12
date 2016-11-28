#pragma once

#include <DirectXMath.h>

#include <MathUtils\MathUtils.h>

class Camera {
public:
	Camera() = default;
	~Camera() = default;
	Camera(const Camera&) = delete;
	const Camera& operator=(const Camera&) = delete;
	Camera(Camera&&) = delete;
	Camera& operator=(Camera&&) = delete;

	// Get/Set world camera position.
	__forceinline DirectX::XMVECTOR GetPosition() const noexcept { return XMLoadFloat3(&mPosition); }
	__forceinline DirectX::XMFLOAT3 GetPosition3f() const noexcept { return mPosition; }
	__forceinline DirectX::XMFLOAT4 GetPosition4f() const noexcept { return DirectX::XMFLOAT4(mPosition.x, mPosition.y, mPosition.z, 1.0f); }
	__forceinline void SetPosition(const float x, const float y, const float z) noexcept { mPosition = DirectX::XMFLOAT3(x, y, z); }
	__forceinline void SetPosition(const DirectX::XMFLOAT3& v) noexcept { mPosition = v; }
	
	// Get camera basis vectors.
	__forceinline DirectX::XMVECTOR GetRight() const noexcept { return XMLoadFloat3(&mRight); }
	__forceinline DirectX::XMFLOAT3 GetRight3f() const noexcept { return mRight; }
	__forceinline DirectX::XMVECTOR GetUp() const noexcept { return XMLoadFloat3(&mUp); }
	__forceinline DirectX::XMFLOAT3 GetUp3f() const noexcept { return mUp; }
	__forceinline DirectX::XMVECTOR GetLook() const noexcept { return XMLoadFloat3(&mLook); }
	__forceinline DirectX::XMFLOAT3 GetLook3f() const noexcept { return mLook; }
		
	// Set frustum.
	void SetLens(const float fovY, const float aspect, const float zn, const float zf) noexcept;

	// Define camera space via LookAt parameters.
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp) noexcept;
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up) noexcept;

	// Get View/Proj matrices.
	__forceinline DirectX::XMMATRIX GetView() const noexcept { return XMLoadFloat4x4(&mView); }
	__forceinline DirectX::XMMATRIX GetProj() const noexcept { return XMLoadFloat4x4(&mProj); }

	__forceinline DirectX::XMMATRIX GetViewProj() const noexcept {
		const DirectX::XMMATRIX viewMatrix = GetView();
		const DirectX::XMMATRIX projMatrix = GetProj();
		return DirectX::XMMatrixMultiply(viewMatrix, projMatrix);
	}

	__forceinline DirectX::XMMATRIX GetTransposeViewProj() const noexcept {
		return DirectX::XMMatrixTranspose(GetViewProj());
	}

	__forceinline const DirectX::XMFLOAT4X4& GetView4x4f() const noexcept { return mView; }
	__forceinline const DirectX::XMFLOAT4X4& GetProj4x4f() const noexcept { return mProj; }	

	void GetView4x4f(DirectX::XMFLOAT4X4& m) const noexcept;
	void GetInvView4x4f(DirectX::XMFLOAT4X4& m) const noexcept;
	void GetProj4x4f(DirectX::XMFLOAT4X4& m) const noexcept;
	void GetInvProj4x4f(DirectX::XMFLOAT4X4& m) const noexcept;

	// Strafe/Walk the camera a distance d.
	void Strafe(const float d) noexcept;
	void Walk(const float d) noexcept;

	// Rotate the camera.
	void Pitch(const float angle) noexcept;
	void RotateY(const float angle) noexcept;

	void UpdateViewMatrix(const float deltaTime) noexcept;

private:
	// Camera coordinate system with coordinates relative to world space.
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	DirectX::XMFLOAT3 mVelocity{ 0.0f, 0.0f, 0.0f };

	// Cache View and Projection matrices.
	DirectX::XMFLOAT4X4 mView{ MathUtils::Identity4x4() };
	DirectX::XMFLOAT4X4 mProj{ MathUtils::Identity4x4() };
	DirectX::XMFLOAT4X4 mInvProj{ MathUtils::Identity4x4() };
};