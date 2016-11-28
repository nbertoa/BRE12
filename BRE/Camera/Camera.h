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
	__forceinline DirectX::XMFLOAT3 GetPosition3f() const noexcept { return mPosition; }
	__forceinline DirectX::XMFLOAT4 GetPosition4f() const noexcept { return DirectX::XMFLOAT4(mPosition.x, mPosition.y, mPosition.z, 1.0f); }
	__forceinline void SetPosition(const float x, const float y, const float z) noexcept { mPosition = DirectX::XMFLOAT3(x, y, z); }
	__forceinline void SetPosition(const DirectX::XMFLOAT3& v) noexcept { mPosition = v; }
	
	// Get camera basis vectors.
	__forceinline DirectX::XMFLOAT3 GetRight3f() const noexcept { return mRight; }
	__forceinline DirectX::XMFLOAT3 GetUp3f() const noexcept { return mUp; }
	__forceinline DirectX::XMFLOAT3 GetLook3f() const noexcept { return mLook; }
		
	// Set frustum.
	void SetLens(const float fovY, const float aspect, const float zn, const float zf) noexcept;

	// Define camera space via LookAt parameters.
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up) noexcept;

	// Get View/Proj matrices.
	__forceinline const DirectX::XMFLOAT4X4& GetView() const noexcept { return mView; }
	__forceinline const DirectX::XMFLOAT4X4& GetInvView() const noexcept { return mInvView; }
	__forceinline const DirectX::XMFLOAT4X4& GetProj() const noexcept { return mProj; }
	__forceinline const DirectX::XMFLOAT4X4& GetInvProj() const noexcept { return mInvProj; }

	__forceinline DirectX::XMMATRIX GetViewProj() const noexcept {
		const DirectX::XMMATRIX viewMatrix = XMLoadFloat4x4(&mView);
		const DirectX::XMMATRIX projMatrix = XMLoadFloat4x4(&mProj);
		return DirectX::XMMatrixMultiply(viewMatrix, projMatrix);
	}

	__forceinline DirectX::XMMATRIX GetTransposeViewProj() const noexcept {
		return DirectX::XMMatrixTranspose(GetViewProj());
	}

	// Strafe/Walk the camera a distance dist.
	void Strafe(const float dist) noexcept;
	void Walk(const float dist) noexcept;

	// Rotate the camera.
	void Pitch(const float angle) noexcept;
	void RotateY(const float angle) noexcept;

	// You should call this function every frame.
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
	DirectX::XMFLOAT4X4 mInvView{ MathUtils::Identity4x4() };
	DirectX::XMFLOAT4X4 mProj{ MathUtils::Identity4x4() };
	DirectX::XMFLOAT4X4 mInvProj{ MathUtils::Identity4x4() };
};