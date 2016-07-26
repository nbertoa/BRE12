#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <cstdint>

class MathHelper {
public:
	// Returns random float in [0, 1).
	static float RandF() noexcept {
		return (float)(rand()) / (float)RAND_MAX;
	}

	// Returns random float in [a, b).
	static float RandF(const float a, const float b) noexcept {
		return a + RandF() * ( b - a);
	}

    static int Rand(const int32_t a, const int32_t b) noexcept
    {
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

	// Returns the polar angle of the point (x,y) in [0, 2 * PI).
	static float AngleFromXY(const float x, const float y) noexcept;

	static DirectX::XMVECTOR SphericalToCartesian(const float radius, const float theta, const float phi) noexcept {
		return DirectX::XMVectorSet(
			radius * sinf(phi) * cosf(theta),
			radius * cosf(phi),
			radius * sinf(phi) * sinf(theta),
			1.0f);
	}

    static DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX M) noexcept {
		// Inverse-transpose is just applied to normals.  So zero out 
		// translation row so that it doesn't get into our inverse-transpose
		// calculation--we don't want the inverse-transpose of the translation.
        DirectX::XMMATRIX A(M);
        A.r[3U] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

        DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(A);
        return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, A));
	}

	static DirectX::XMMATRIX GetTranspose(const DirectX::XMFLOAT4X4& m) noexcept {
		const DirectX::XMMATRIX matrix = XMLoadFloat4x4(&m);
		return DirectX::XMMatrixTranspose(matrix);
	}

	static DirectX::XMMATRIX GetTransposeViewProj(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj) noexcept {
		const DirectX::XMMATRIX viewMatrix = XMLoadFloat4x4(&view);
		const DirectX::XMMATRIX projMatrix = XMLoadFloat4x4(&proj);
		return DirectX::XMMatrixTranspose(DirectX::XMMatrixMultiply(viewMatrix, projMatrix));
	}

    static DirectX::XMFLOAT4X4 Identity4x4() noexcept {
        static DirectX::XMFLOAT4X4 I(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

        return I;
    }

    static DirectX::XMVECTOR RandUnitVec3() noexcept;
    static DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n) noexcept;

	static const float Infinity;
	static const float Pi;
};

