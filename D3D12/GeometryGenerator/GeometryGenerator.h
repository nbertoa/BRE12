//////////////////////////////////////////////////////////////////////////////////////////
//   
// Defines a static class for procedurally generating the geometry of 
// common mathematical objects.
//
// All triangles are generated "outward" facing.  If you want "inward" 
// facing triangles (for example, if you want to place the camera inside
// a sphere to simulate a sky), you will need to:
//   1. Change the Direct3D cull mode or manually reverse the winding order.
//   2. Invert the normal.
//   3. Update the texture coordinates and tangent vectors.
//////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <DirectXMath.h>
#include <vector>

class GeometryGenerator {
public:
	struct Vertex {
		Vertex() {}
        Vertex(
            const DirectX::XMFLOAT3& p, 
            const DirectX::XMFLOAT3& n, 
            const DirectX::XMFLOAT3& t, 
            const DirectX::XMFLOAT2& uv) :
			mPosition(p),
			mNormal(n),
			mTangentU(t),
			mTexC(uv){}
		Vertex(
			const float px, const float py, const float pz,
			const float nx, const float ny, const float nz,
			const float tx, const float ty, const float tz,
			const float u, const float v) :
			mPosition(px,py,pz),
			mNormal(nx,ny,nz),
			mTangentU(tx, ty, tz),
			mTexC(u,v){}

		DirectX::XMFLOAT3 mPosition = {0.0f, 0.0f, 0.0f};
        DirectX::XMFLOAT3 mNormal = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 mTangentU = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT2 mTexC = { 0.0f, 0.0f };
	};

	struct MeshData {
		std::vector<Vertex> mVertices;
        std::vector<std::uint32_t> mIndices32;

        std::vector<std::uint16_t>& GetIndices16() {
			if(mIndices16.empty()) {
				mIndices16.resize(mIndices32.size());
				for (std::size_t i = 0; i < mIndices32.size(); ++i) {
					mIndices16[i] = (std::uint16_t) mIndices32[i];
				}
			}

			return mIndices16;
        }

	private:
		std::vector<std::uint16_t> mIndices16 = {};
	};

	// Creates a box centered at the origin with the given dimensions, where each
    // face has m rows and n columns of vertices.
    MeshData CreateBox(const float width, const float height, const float depth, const std::uint32_t numSubdivisions);

	// Creates a sphere centered at the origin with the given radius.  The
	// slices and stacks parameters control the degree of tessellation.
    MeshData CreateSphere(const float radius, const std::uint32_t sliceCount, const std::uint32_t stackCount);

	// Creates a geosphere centered at the origin with the given radius.  The
	// depth controls the level of tessellation.
    MeshData CreateGeosphere(const float radius, const std::uint32_t numSubdivisions);

	// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	// The bottom and top radius can vary to form various cone shapes rather than true
	// cylinders.  The slices and stacks parameters control the degree of tessellation.
    MeshData CreateCylinder(const float bottomRadius, const float topRadius, const float height, const std::uint32_t sliceCount, const std::uint32_t stackCount);

	// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	// at the origin with the specified width and depth.
    MeshData CreateGrid(const float width, const float depth, const std::uint32_t m, const std::uint32_t n);

	// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
    MeshData CreateQuad(const float x, const float y, const float w, const float h, const float depth);

private:
	void Subdivide(MeshData& meshData);
    Vertex MidPoint(const Vertex& v0, const Vertex& v1);
    void BuildCylinderTopCap(const float topRadius, const float height, const std::uint32_t sliceCount, MeshData& meshData);
    void BuildCylinderBottomCap(const float bottomRadius, const float height, const std::uint32_t sliceCount, MeshData& meshData);
};

