#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <cmath>
#include <cstdint>

#include <Utils\DebugUtils.h>

namespace BRE {
///
/// @brief Responsible of mathematical utilities for vectors, matrices, etc
///
class MathUtils {
public:
    MathUtils() = delete;
    ~MathUtils() = delete;
    MathUtils(const MathUtils&) = delete;
    const MathUtils& operator=(const MathUtils&) = delete;
    MathUtils(MathUtils&&) = delete;
    MathUtils& operator=(MathUtils&&) = delete;

    ///
    /// @brief Computes a random float in an interval
    /// @param bottomValue Bottom value
    /// @param topValue Top Value
    /// @return Random value
    ///
    static float RandomFloatInInterval(const float bottomValue,
                                       const float topValue) noexcept
    {
        BRE_ASSERT(bottomValue < topValue);
        const float randomBetweenZeroAndOne = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        return bottomValue + randomBetweenZeroAndOne * (topValue - bottomValue);
    }

    ///
    /// @brief Computes random integer in an interval
    /// @param a First integer in invertal
    /// @param b Second integer in interval
    /// @return Random integer
    ///
    static int RandomIntegerInInterval(const int32_t a,
                                       const int32_t b) noexcept
    {
        return a + rand() % ((b - a) + 1);
    }

    ///
    /// @brief Computes the minimum of 2 values
    /// @param a First value
    /// @param b Second value
    /// @return Minimum value
    ///
    template<typename T>
    static T Min(const T& a,
                 const T& b) noexcept
    {
        return a < b ? a : b;
    }

    ///
    /// @brief Computes the maximum of 2 values
    /// @param a First value
    /// @param b Second value
    /// @return Maximum value
    ///
    template<typename T>
    static T Max(const T& a,
                 const T& b) noexcept
    {
        return a > b ? a : b;
    }

    ///
    /// @brief Computes the linear interpolation
    /// @param a First value
    /// @param b Second value
    /// @param t Interpolation factor between 0 and 1
    /// @return Interpolated value
    ///
    template<typename T>
    static T Lerp(const T& a,
                  const T& b,
                  const float t) noexcept
    {
        BRE_ASSERT(0.0f < t || AreEqual(0.0f, t));
        BRE_ASSERT(t < 1.0f || AreEqual(1.0f, t));
        return a + (b - a) * t;
    }

    ///
    /// @brief Clamp a value
    /// @param x Value
    /// @param low Low limit. It must be less or equal than @p high.
    /// @param high High limit. It must be greater of equal than @p low.
    /// @return Clamped value
    ///
    template<typename T>
    static T Clamp(const T& x,
                   const T& low,
                   const T& high) noexcept
    {
        BRE_ASSERT(low < high || AreEqual(low, high));
        return x < low ? low : (x > high ? high : x);
    }

    ///
    /// @brief Get transpose matrix
    /// @param matrix Matrix to compute transpose
    /// @return Transposed matrix
    ///
    static DirectX::XMMATRIX GetTransposeMatrix(const DirectX::XMFLOAT4X4& matrix) noexcept
    {
        const DirectX::XMMATRIX xmMatrix = XMLoadFloat4x4(&matrix);
        return DirectX::XMMatrixTranspose(xmMatrix);
    }

    ///
    /// @brief Store transpose matrix
    /// @param sourceMatrix Matrix to compute transpose
    /// @param destinationMatrix Output transpose matrix
    ///
    static void StoreTransposeMatrix(const DirectX::XMFLOAT4X4& sourceMatrix,
                                     DirectX::XMFLOAT4X4& destinationMatrix) noexcept
    {
        const DirectX::XMMATRIX xmMatrix = GetTransposeMatrix(sourceMatrix);
        DirectX::XMStoreFloat4x4(&destinationMatrix, xmMatrix);
    }

    ///
    /// @brief Get inverse matrix
    /// @param matrix Matrix to compute inverse
    /// @return Inverted matrix
    ///
    static DirectX::XMMATRIX GetInverseMatrix(const DirectX::XMFLOAT4X4& matrix) noexcept
    {
        const DirectX::XMMATRIX xmMatrix = XMLoadFloat4x4(&matrix);
        return DirectX::XMMatrixInverse(nullptr, xmMatrix);
    }

    ///
    /// @brief Stores inverse matrix
    /// @param sourceMatrix Matrix to compute inverse
    /// @param destinationMatrix Output inverted matrix
    ///
    static void StoreInverseMatrix(const DirectX::XMFLOAT4X4& sourceMatrix,
                                   DirectX::XMFLOAT4X4& destinationMatrix) noexcept
    {
        const DirectX::XMMATRIX xmMatrix = GetInverseMatrix(sourceMatrix);
        DirectX::XMStoreFloat4x4(&destinationMatrix, xmMatrix);
    }

    ///
    /// @brief Get inverse transpose matrix
    /// @param matrix Matrix to compute inverse transpose
    /// @return Inverse transposed matrix
    ///
    static DirectX::XMMATRIX GetInverseTransposeMatrix(const DirectX::XMFLOAT4X4& matrix) noexcept
    {
        const DirectX::XMMATRIX xmInverseMatrix = GetInverseMatrix(matrix);
        DirectX::XMFLOAT4X4 inverseMatrix;
        DirectX::XMStoreFloat4x4(&inverseMatrix, xmInverseMatrix);
        return GetTransposeMatrix(inverseMatrix);
    }

    ///
    /// @brief Stores inverse transpose matrix
    /// @param sourceMatrix Matrix to compute inverse transpose
    /// @param destinationMatrix Output inverse transposed matrix
    ///
    static void StoreInverseTransposeMatrix(const DirectX::XMFLOAT4X4& sourceMatrix,
                                            DirectX::XMFLOAT4X4& destinationMatrix) noexcept
    {
        const DirectX::XMMATRIX xmMatrix = GetInverseTransposeMatrix(sourceMatrix);
        DirectX::XMStoreFloat4x4(&destinationMatrix, xmMatrix);
    }

    ///
    /// @brief Computes a matrix
    /// @param m Output matrix
    /// @param tx X translation
    /// @param ty Y translation
    /// @param tz Z translation
    /// @param sx X scale
    /// @param sy Y scale
    /// @param sz Z scale
    /// @param rx X rotation
    /// @param ry Y rotation
    /// @param rz Z rotation
    /// @return Transposed matrix
    ///
    static void ComputeMatrix(DirectX::XMFLOAT4X4& m,
                              const float tx,
                              const float ty,
                              const float tz,
                              const float sx = 1.0f,
                              const float sy = 1.0f,
                              const float sz = 1.0f,
                              const float rx = 0.0f,
                              const float ry = 0.0f,
                              const float rz = 0.0f) noexcept;

    ///
    /// @brief Get identity matrix of 4x4
    /// @return Indentity matrix
    ///
    static DirectX::XMFLOAT4X4 GetIdentity4x4Matrix() noexcept
    {
        DirectX::XMFLOAT4X4 identityMatrix(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

        return identityMatrix;
    }

    ///
    /// @brief Checks if two floats are equal or not.
    /// @return True if they are equal. Otherwise, false.
    ///
    static float AreEqual(const float f1,
                          const float f2)
    {
        return fabs(f1 - f2) < 1.e-5f;
    }

    ///
    /// @brief Checks if two XMFLOAT2 are equal or not.
    /// @param p1 First XMFLOAT2
    /// @param p2 Second XMFLOAT2
    /// @return True if they are equal. Otherwise, false
    ///
    static bool AreEqual(const DirectX::XMFLOAT2& p1,
                         const DirectX::XMFLOAT2& p2)
    {
        const DirectX::XMVECTOR xmP1 = DirectX::XMLoadFloat2(&p1);
        const DirectX::XMVECTOR xmP2 = DirectX::XMLoadFloat2(&p2);

        return DirectX::XMVector2Equal(xmP1, xmP2);
    }

    ///
    /// @brief Checks if two XMFLOAT3 are equal or not.
    /// @param p1 First XMFLOAT3
    /// @param p2 Second XMFLOAT3
    /// @return True if they are equal. Otherwise, false
    ///
    static bool AreEqual(const DirectX::XMFLOAT3& p1,
                         const DirectX::XMFLOAT3& p2)
    {
        const DirectX::XMVECTOR xmP1 = DirectX::XMLoadFloat3(&p1);
        const DirectX::XMVECTOR xmP2 = DirectX::XMLoadFloat3(&p2);

        return DirectX::XMVector3Equal(xmP1, xmP2);
    }

    ///
    /// @brief Checks if two XMFLOAT4 are equal or not.
    /// @param p1 First XMFLOAT4
    /// @param p2 Second XMFLOAT4
    /// @return True if they are equal. Otherwise, false
    ///
    static bool AreEqual(const DirectX::XMFLOAT4& p1,
                         const DirectX::XMFLOAT4& p2)
    {
        const DirectX::XMVECTOR xmP1 = DirectX::XMLoadFloat4(&p1);
        const DirectX::XMVECTOR xmP2 = DirectX::XMLoadFloat4(&p2);

        return DirectX::XMVector4Equal(xmP1, xmP2);
    }

    ///
    /// @brief Checks if two XMFLOAT4X4 are equal or not.
    /// @param p1 First XMFLOAT4X4
    /// @param p2 Second XMFLOAT4X4
    /// @return True if they are equal. Otherwise, false
    ///
    static bool AreEqual(const DirectX::XMFLOAT4X4& p1,
                         const DirectX::XMFLOAT4X4& p2)
    {
        return
            AreEqual(p1._11, p2._11) &&
            AreEqual(p1._12, p2._12) &&
            AreEqual(p1._13, p2._13) &&
            AreEqual(p1._14, p2._14) &&
            AreEqual(p1._21, p2._21) &&
            AreEqual(p1._22, p2._22) &&
            AreEqual(p1._23, p2._23) &&
            AreEqual(p1._24, p2._24) &&
            AreEqual(p1._31, p2._31) &&
            AreEqual(p1._32, p2._32) &&
            AreEqual(p1._33, p2._33) &&
            AreEqual(p1._34, p2._34) &&
            AreEqual(p1._41, p2._41) &&
            AreEqual(p1._42, p2._42) &&
            AreEqual(p1._43, p2._43) &&
            AreEqual(p1._44, p2._44);
    }

    static const float Infinity;
    static const float Pi;
};
}