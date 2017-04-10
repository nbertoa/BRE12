#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <cstdint>

#include <Utils\DebugUtils.h>

class MathUtils {
public:
	MathUtils() = delete;
	~MathUtils() = delete;
	MathUtils(const MathUtils&) = delete;
	const MathUtils& operator=(const MathUtils&) = delete;
	MathUtils(MathUtils&&) = delete;
	MathUtils& operator=(MathUtils&&) = delete;
	
	static float RandomFloatInInverval(const float bottomValue, const float topValue) noexcept {
		ASSERT(bottomValue < topValue);
		const float randomBetweenZeroAndOne = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		return bottomValue + randomBetweenZeroAndOne * (topValue - bottomValue);
	}

    static int RandomIntegerInInterval(const int32_t a, const int32_t b) noexcept {
        return a + rand() % ((b - a) + 1);
    }

	template<typename T>
	static T Min(const T& a, const T& b) noexcept {
		return a < b ? a : b;
	}

	template<typename T>
	static T Max(const T& a, const T& b) noexcept {
		return a > b ? a : b;
	}
	 
	template<typename T>
	static T Lerp(const T& a, const T& b, const float t) noexcept {
		return a + (b - a) * t;
	}

	template<typename T>
	static T Clamp(const T& x, const T& low, const T& high) noexcept {
		return x < low ? low : (x > high ? high : x); 
	}

	static DirectX::XMMATRIX GetTransposeMatrix(const DirectX::XMFLOAT4X4& matrix) noexcept {
		const DirectX::XMMATRIX xmMatrix = XMLoadFloat4x4(&matrix);
		return DirectX::XMMatrixTranspose(xmMatrix);
	}

	static void StoreTransposeMatrix(
		const DirectX::XMFLOAT4X4& sourceMatrix, 
		DirectX::XMFLOAT4X4& destinationMatrix) noexcept 
	{
		const DirectX::XMMATRIX xmMatrix = GetTransposeMatrix(sourceMatrix);
		DirectX::XMStoreFloat4x4(&destinationMatrix, xmMatrix);
	}

	static DirectX::XMMATRIX GetInverseMatrix(const DirectX::XMFLOAT4X4& matrix) noexcept {
		const DirectX::XMMATRIX xmMatrix = XMLoadFloat4x4(&matrix);
		return DirectX::XMMatrixInverse(nullptr, xmMatrix);
	}

	static void StoreInverseMatrix(
		const DirectX::XMFLOAT4X4& sourceMatrix,
		DirectX::XMFLOAT4X4& destinationMatrix) noexcept
	{
		const DirectX::XMMATRIX xmMatrix = GetInverseMatrix(sourceMatrix);
		DirectX::XMStoreFloat4x4(&destinationMatrix, xmMatrix);
	}

	static DirectX::XMMATRIX GetInverseTransposeMatrix(const DirectX::XMFLOAT4X4& matrix) noexcept {
		const DirectX::XMMATRIX xmInverseMatrix = GetInverseMatrix(matrix);
		DirectX::XMFLOAT4X4 inverseMatrix;
		DirectX::XMStoreFloat4x4(&inverseMatrix, xmInverseMatrix);
		return GetTransposeMatrix(inverseMatrix);
	}

	static void StoreInverseTransposeMatrix(
		const DirectX::XMFLOAT4X4& sourceMatrix,
		DirectX::XMFLOAT4X4& destinationMatrix) noexcept
	{
		const DirectX::XMMATRIX xmMatrix = GetInverseTransposeMatrix(sourceMatrix);
		DirectX::XMStoreFloat4x4(&destinationMatrix, xmMatrix);
	}

	static void ComputeMatrix(
		DirectX::XMFLOAT4X4& m,
		const float tx,
		const float ty,
		const float tz,
		const float sx = 1.0f,
		const float sy = 1.0f,
		const float sz = 1.0f,
		const float rx = 0.0f,
		const float ry = 0.0f,
		const float rz = 0.0f) noexcept;

    static DirectX::XMFLOAT4X4 GetIdentity4x4Matrix() noexcept {
        DirectX::XMFLOAT4X4 identityMatrix(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

        return identityMatrix;
    }

	static const float Infinity;
	static const float Pi;
};

