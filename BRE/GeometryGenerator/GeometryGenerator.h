#pragma once

#include <DirectXMath.h>
#include <vector>

// Defines a static class for procedurally generating the geometry of 
// common mathematical objects.
//
// All triangles are generated "outward" facing.  If you want "inward" 
// facing triangles (for example, if you want to place the camera inside
// a sphere to simulate a sky), you will need to:
//   1. Change the Direct3D cull mode or manually reverse the winding order.
//   2. Invert the normal.
//   3. Update the texture coordinates and tangent vectors.
namespace GeometryGenerator {
	struct Vertex {
		Vertex() = default;
		Vertex(const DirectX::XMFLOAT3& p, const DirectX::XMFLOAT3& n, const DirectX::XMFLOAT3& t, const DirectX::XMFLOAT2& uv);

		DirectX::XMFLOAT3 mPosition = {0.0f, 0.0f, 0.0f};
        DirectX::XMFLOAT3 mNormal = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 mTangentU = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT2 mTexC = { 0.0f, 0.0f };
	};

	struct MeshData {
		std::vector<Vertex> mVertices;
        std::vector<std::uint32_t> mIndices32;
		std::vector<std::uint16_t>& GetIndices16() noexcept;

	private:
		std::vector<std::uint16_t> mIndices16{};
	};

	// Creates a box centered at the origin with the given dimensions, where each
    // face has m rows and n columns of vertices.
    void CreateBox(const float width, const float height, const float depth, const std::uint32_t numSubdivisions, MeshData& meshData) noexcept;

	// Creates a sphere centered at the origin with the given radius.  The
	// slices and stacks parameters control the degree of tessellation.
	void CreateSphere(const float radius, const std::uint32_t sliceCount, const std::uint32_t stackCount, MeshData& meshData) noexcept;

	// Creates a geosphere centered at the origin with the given radius.  The
	// depth controls the level of tessellation.
	void CreateGeosphere(const float radius, const std::uint32_t numSubdivisions, MeshData& meshData) noexcept;

	// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	// The bottom and top radius can vary to form various cone shapes rather than true
	// cylinders.  The slices and stacks parameters control the degree of tessellation.
	void CreateCylinder(
		const float bottomRadius, 
		const float topRadius, 
		const float height, 
		const std::uint32_t sliceCount, 
		const std::uint32_t stackCount, 
		MeshData& meshData) noexcept;

	// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	// at the origin with the specified width and depth.
	void CreateGrid(const float width, const float depth, const std::uint32_t m, const std::uint32_t n, MeshData& meshData) noexcept;

	// Creates a quad aligned with the screen.  This is useful for post-processing and screen effects.
	void CreateQuad(const float x, const float y, const float w, const float h, const float depth, MeshData& meshData) noexcept;

	// Creates a fullscreen quad aligned with the screen.  This is useful for post-processing and screen effects.
	// Positions will be in NDC
	void CreateFullscreenQuad( MeshData& meshData) noexcept;
}

