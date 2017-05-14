#pragma once

#include <DirectXMath.h>
#include <vector>

namespace BRE {
///
/// @brief Responsible to generate procedurally the geometry of common mathematical objects.
///
/// All triangles are generated outward facing.
///
namespace GeometryGenerator {
struct Vertex {
    Vertex() = default;

    ///
    /// @brief Vertex constructor
    /// @param position Position
    /// @param normal Normal
    /// @param tangent Tangent
    /// @param uv UV
    ///
    explicit Vertex(const DirectX::XMFLOAT3& position,
                    const DirectX::XMFLOAT3& normal,
                    const DirectX::XMFLOAT3& tangent,
                    const DirectX::XMFLOAT2& uv);

    ~Vertex() = default;
    Vertex(const Vertex&) = default;
    Vertex(Vertex&&) = default;
    Vertex& operator=(Vertex&&) = default;

    DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 mNormal = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 mTangent = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT2 mUV = { 0.0f, 0.0f };
};

struct MeshData {
    std::vector<Vertex> mVertices;
    std::vector<std::uint32_t> mIndices32;

    ///
    /// @brief Get indices of 16 bytes each
    /// @return List of indices
    ///
    std::vector<std::uint16_t>& GetIndices16() noexcept;

private:
    std::vector<std::uint16_t> mIndices16{};
};

///
/// @brief Creates a box centered at the origin
/// @param width Width
/// @param height Height
/// @param depth Depth
/// @param numSubdivisions Number of subdivisions. Controls the degree of tessellation
/// @param meshData Output mesh data of the box
///
void CreateBox(const float width,
               const float height,
               const float depth,
               const std::uint32_t numSubdivisions,
               MeshData& meshData) noexcept;

///
/// @brief Creates a sphere centered at the origin
/// @param radius Radius
/// @param sliceCount Slice count. Controls the degree of tessellation.
/// @param stackCount Stack count. Controls the degree of tessellation.
/// @param meshData Output mesh data of the sphere
///
void CreateSphere(const float radius,
                  const std::uint32_t sliceCount,
                  const std::uint32_t stackCount,
                  MeshData& meshData) noexcept;

///
/// @brief Creates a geosphere centered at the origin
/// @param radius Radius
/// @param numSubdivisions Number of subdivisions. Controls the degree of tessellation
/// @param meshData Output mesh data of the geosphere
///
void CreateGeosphere(const float radius,
                     const std::uint32_t numSubdivisions,
                     MeshData& meshData) noexcept;

///
/// @brief Creates a cylinder parallel to the y-axis, and centered about the origin.  
///
/// The bottom and top radius can vary to form various cone shapes rather than true
/// cylinders. The slices and stacks parameters control the degree of tessellation.
///
/// @param bottomRadius Bottom radius
/// @param topRadius Top radius
/// @param height Height
/// @param sliceCount Slice count.
/// @param stackCount Stack count.
/// @param meshData Output mesh data of the cylinder
///
void CreateCylinder(const float bottomRadius,
                    const float topRadius,
                    const float height,
                    const std::uint32_t sliceCount,
                    const std::uint32_t stackCount,
                    MeshData& meshData) noexcept;

///
/// @brief Creates a grid in the xz-plane.
///
/// Creates a row x columns grid in the xz-plane with rows and columns, centered
/// at the origin with the specified width and depth.
///
/// @param width Width
/// @param height Height
/// @param rows Rows
/// @param columns Columns
/// @param meshData Output mesh data of the grid
///
void CreateGrid(const float width,
                const float depth,
                const std::uint32_t rows,
                const std::uint32_t columns,
                MeshData& meshData) noexcept;
}
}

