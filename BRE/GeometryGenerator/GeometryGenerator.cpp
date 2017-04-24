#include "GeometryGenerator.h"

#include <algorithm>

#include <Utils\DebugUtils.h>

using namespace DirectX;

namespace {
	GeometryGenerator::Vertex GetMiddlePoint(
		const GeometryGenerator::Vertex& vertex0, 
		const GeometryGenerator::Vertex& vertex1) noexcept 
	{
		const XMVECTOR point0(XMLoadFloat3(&vertex0.mPosition));
		const XMVECTOR point1(XMLoadFloat3(&vertex1.mPosition));

		const XMVECTOR normal0(XMLoadFloat3(&vertex0.mNormal));
		const XMVECTOR normal1(XMLoadFloat3(&vertex1.mNormal));

		const XMVECTOR tangent0(XMLoadFloat3(&vertex0.mTangent));
		const XMVECTOR tangent1(XMLoadFloat3(&vertex1.mTangent));

		const XMVECTOR uv0(XMLoadFloat2(&vertex0.mUV));
		const XMVECTOR uv1(XMLoadFloat2(&vertex1.mUV));

		// Compute the midpoints of all the attributes. Vectors need to be normalized
		// since linear interpolating can make them not unit length.  
		const XMVECTOR position(0.5f * (point0 + point1));
		const XMVECTOR normal(XMVector3Normalize(0.5f * (normal0 + normal1)));
		const XMVECTOR tangent(XMVector3Normalize(0.5f * (tangent0 + tangent1)));
		const XMVECTOR uv(0.5f * (uv0 + uv1));

		GeometryGenerator::Vertex middleVertex;
		XMStoreFloat3(&middleVertex.mPosition, position);
		XMStoreFloat3(&middleVertex.mNormal, normal);
		XMStoreFloat3(&middleVertex.mTangent, tangent);
		XMStoreFloat2(&middleVertex.mUV, uv);

		return middleVertex;
	}

	void Subdivide(GeometryGenerator::MeshData& meshData) noexcept {
		// Save a copy of the input geometry.
		GeometryGenerator::MeshData inputCopy{ meshData };

		meshData.mVertices.resize(0U);
		meshData.mIndices32.resize(0U);

		//       v1
		//       *
		//      / \
		//     /   \
		//  m0*-----*m1
		//   / \   / \
		//  /   \ /   \
		// *-----*-----*
		// v0    m2     v2

		const std::uint32_t numTriangles{ static_cast<std::uint32_t>(inputCopy.mIndices32.size()) / 3U };
		for (std::uint32_t i = 0; i < numTriangles; ++i) {
			const std::uint32_t i3{ i * 3U };
			const GeometryGenerator::Vertex v0{ inputCopy.mVertices[inputCopy.mIndices32[i3 + 0U]] };
			const GeometryGenerator::Vertex v1{ inputCopy.mVertices[inputCopy.mIndices32[i3 + 1U]] };
			const GeometryGenerator::Vertex v2{ inputCopy.mVertices[inputCopy.mIndices32[i3 + 2U]] };

			//
			// Generate the midpoints.
			//

			const GeometryGenerator::Vertex m0{ GetMiddlePoint(v0, v1) };
			const GeometryGenerator::Vertex m1{ GetMiddlePoint(v1, v2) };
			const GeometryGenerator::Vertex m2{ GetMiddlePoint(v0, v2) };

			//
			// Add new geometry.
			//

			meshData.mVertices.push_back(v0); // 0
			meshData.mVertices.push_back(v1); // 1
			meshData.mVertices.push_back(v2); // 2
			meshData.mVertices.push_back(m0); // 3
			meshData.mVertices.push_back(m1); // 4
			meshData.mVertices.push_back(m2); // 5

			const std::uint32_t i6 = i * 6U;

			meshData.mIndices32.push_back(i6 + 0U);
			meshData.mIndices32.push_back(i6 + 3U);
			meshData.mIndices32.push_back(i6 + 5U);

			meshData.mIndices32.push_back(i6 + 3U);
			meshData.mIndices32.push_back(i6 + 4U);
			meshData.mIndices32.push_back(i6 + 5U);

			meshData.mIndices32.push_back(i6 + 5U);
			meshData.mIndices32.push_back(i6 + 4U);
			meshData.mIndices32.push_back(i6 + 2U);

			meshData.mIndices32.push_back(i6 + 3U);
			meshData.mIndices32.push_back(i6 + 1U);
			meshData.mIndices32.push_back(i6 + 4U);
		}
	}

	void BuildCylinderTopCap(
		const float topRadius, 
		const float height, 
		const std::uint32_t sliceCount, 
		GeometryGenerator::MeshData& meshData) noexcept 
	{
		const std::uint32_t baseIndex{ static_cast<std::uint32_t>(meshData.mVertices.size()) };

		const float y{ 0.5f * height };
		const float dTheta{ 2.0f * XM_PI / sliceCount };

		// Duplicate cap ring mVertices because the texture coordinates and normals differ.
		for (std::uint32_t i = 0U; i <= sliceCount; ++i) {
			const float x{ topRadius * cosf(i * dTheta) };
			const float z{ topRadius * sinf(i * dTheta) };

			// Scale down by the height to try and make top cap texture coordinate area
			// proportional to base.
			const float u{ x / height + 0.5f };
			const float v{ z / height + 0.5f };

			meshData.mVertices.push_back(GeometryGenerator::Vertex{ XMFLOAT3{x, y, z}, XMFLOAT3{0.0f, 1.0f, 0.0f}, XMFLOAT3{1.0f, 0.0f, 0.0f}, XMFLOAT2{u, v} });
		}

		// Cap center vertex.
		meshData.mVertices.push_back(GeometryGenerator::Vertex{ XMFLOAT3{0.0f, y, 0.0f}, XMFLOAT3{0.0f, 1.0f, 0.0f}, XMFLOAT3{1.0f, 0.0f, 0.0f}, XMFLOAT2{0.5f, 0.5f} });

		// Index of center vertex.
		const std::uint32_t centerIndex{ static_cast<std::uint32_t>(meshData.mVertices.size()) - 1U };

		for (std::uint32_t i = 0U; i < sliceCount; ++i) {
			meshData.mIndices32.push_back(centerIndex);
			meshData.mIndices32.push_back(baseIndex + i + 1U);
			meshData.mIndices32.push_back(baseIndex + i);
		}
	}

	void BuildCylinderBottomCap(
		const float bottomRadius, 
		const float height, 
		const std::uint32_t sliceCount, 
		GeometryGenerator::MeshData& meshData) noexcept 
	{
		// 
		// Build bottom cap.
		//

		const std::uint32_t baseIndex{ static_cast<std::uint32_t>(meshData.mVertices.size()) };
		const float y{ -0.5f * height };

		// mVertices of ring
		const float dTheta{ 2.0f * XM_PI / sliceCount };
		for (std::uint32_t i = 0U; i <= sliceCount; ++i) {
			const float x{ bottomRadius * cosf(i * dTheta) };
			const float z{ bottomRadius * sinf(i * dTheta) };

			// Scale down by the height to try and make top cap texture coord area
			// proportional to base.
			const float u{ x / height + 0.5f };
			const float v{ z / height + 0.5f };

			meshData.mVertices.push_back(GeometryGenerator::Vertex{ XMFLOAT3{x, y, z}, XMFLOAT3{0.0f, -1.0f, 0.0f}, XMFLOAT3{1.0f, 0.0f, 0.0f}, XMFLOAT2{u, v} });
		}

		// Cap center vertex.
		meshData.mVertices.push_back(GeometryGenerator::Vertex{ XMFLOAT3{0.0f, y, 0.0f}, XMFLOAT3{0.0f, -1.0f, 0.0f}, XMFLOAT3{1.0f, 0.0f, 0.0f}, XMFLOAT2{0.5f, 0.5f} });

		// Cache the index of center vertex.
		const std::uint32_t centerIndex{ static_cast<std::uint32_t>(meshData.mVertices.size()) - 1U };

		for (std::uint32_t i = 0U; i < sliceCount; ++i) {
			meshData.mIndices32.push_back(centerIndex);
			meshData.mIndices32.push_back(baseIndex + i);
			meshData.mIndices32.push_back(baseIndex + i + 1U);
		}
	}
}

namespace GeometryGenerator {
	Vertex::Vertex(
		const XMFLOAT3& position, 
		const XMFLOAT3& normal, 
		const XMFLOAT3& tangent, 
		const XMFLOAT2& uv) 
		: mPosition(position)
		, mNormal(normal)
		, mTangent(tangent)
		, mUV(uv) 
	{
	}

	std::vector<std::uint16_t>& MeshData::GetIndices16() noexcept {
		if (mIndices16.empty()) {
			mIndices16.resize(mIndices32.size());
			for (std::size_t i = 0; i < mIndices32.size(); ++i) {
				mIndices16[i] = static_cast<std::uint16_t>(mIndices32[i]);
			}
		}

		return mIndices16;
	}

	void CreateBox(
		const float width, 
		const float height, 
		const float depth,
		const std::uint32_t numSubdivisions, 
		MeshData& meshData) noexcept 
	{
		//
		// Create the vertices.
		//

		Vertex v[24];

		float w2{ 0.5f * width };
		float h2{ 0.5f * height };
		float d2{ 0.5f * depth };

		// Fill in the front face vertex data.
		v[0] = Vertex(XMFLOAT3{ -w2, -h2, -d2 }, XMFLOAT3{ 0.0f, 0.0f, -1.0f }, XMFLOAT3{ 1.0f, 0.0f, 0.0f, }, XMFLOAT2{ 0.0f, 1.0f });
		v[1] = Vertex(XMFLOAT3{ -w2, +h2, -d2 }, XMFLOAT3{ 0.0f, 0.0f, -1.0f }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT2{ 0.0f, 0.0f });
		v[2] = Vertex(XMFLOAT3{ +w2, +h2, -d2 }, XMFLOAT3{ 0.0f, 0.0f, -1.0f }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT2{ 1.0f, 0.0f });
		v[3] = Vertex(XMFLOAT3{ +w2, -h2, -d2 }, XMFLOAT3{ 0.0f, 0.0f, -1.0f }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT2{ 1.0f, 1.0f });

		// Fill in the back face vertex data.
		v[4] = Vertex(XMFLOAT3{ -w2, -h2, +d2 }, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT2{ 1.0f, 1.0f });
		v[5] = Vertex(XMFLOAT3{ +w2, -h2, +d2 }, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT2{ 0.0f, 1.0f });
		v[6] = Vertex(XMFLOAT3{ +w2, +h2, +d2 }, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT2{ 0.0f, 0.0f });
		v[7] = Vertex(XMFLOAT3{ -w2, +h2, +d2 }, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT2{ 1.0f, 0.0f });

		// Fill in the top face vertex data.
		v[8] = Vertex(XMFLOAT3{ -w2, +h2, -d2 }, XMFLOAT3{ 0.0f, 1.0f, 0.0f }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT2{ 0.0f, 1.0f });
		v[9] = Vertex(XMFLOAT3{ -w2, +h2, +d2 }, XMFLOAT3{ 0.0f, 1.0f, 0.0f }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT2{ 0.0f, 0.0f });
		v[10] = Vertex(XMFLOAT3{ +w2, +h2, +d2 }, XMFLOAT3{ 0.0f, 1.0f, 0.0f }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT2{ 1.0f, 0.0f });
		v[11] = Vertex(XMFLOAT3{ +w2, +h2, -d2 }, XMFLOAT3{ 0.0f, 1.0f, 0.0f }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT2{ 1.0f, 1.0f });

		// Fill in the bottom face vertex data.
		v[12] = Vertex(XMFLOAT3{ -w2, -h2, -d2 }, XMFLOAT3{ 0.0f, -1.0f, 0.0f }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT2{ 1.0f, 1.0f });
		v[13] = Vertex(XMFLOAT3{ +w2, -h2, -d2 }, XMFLOAT3{ 0.0f, -1.0f, 0.0f }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT2{ 0.0f, 1.0f });
		v[14] = Vertex(XMFLOAT3{ +w2, -h2, +d2 }, XMFLOAT3{ 0.0f, -1.0f, 0.0f }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT2{ 0.0f, 0.0f });
		v[15] = Vertex(XMFLOAT3{ -w2, -h2, +d2 }, XMFLOAT3{ 0.0f, -1.0f, 0.0f }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT2{ 1.0f, 0.0f });

		// Fill in the left face vertex data.
		v[16] = Vertex(XMFLOAT3{ -w2, -h2, +d2 }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, -1.0f }, XMFLOAT2{ 0.0f, 1.0f });
		v[17] = Vertex(XMFLOAT3{ -w2, +h2, +d2 }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, -1.0f }, XMFLOAT2{ 0.0f, 0.0f });
		v[18] = Vertex(XMFLOAT3{ -w2, +h2, -d2 }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, -1.0f }, XMFLOAT2{ 1.0f, 0.0f });
		v[19] = Vertex(XMFLOAT3{ -w2, -h2, -d2 }, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, -1.0f }, XMFLOAT2{ 1.0f, 1.0f });

		// Fill in the right face vertex data.
		v[20] = Vertex(XMFLOAT3{ +w2, -h2, -d2 }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, XMFLOAT2{ 0.0f, 1.0f });
		v[21] = Vertex(XMFLOAT3{ +w2, +h2, -d2 }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, XMFLOAT2{ 0.0f, 0.0f });
		v[22] = Vertex(XMFLOAT3{ +w2, +h2, +d2 }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, XMFLOAT2{ 1.0f, 0.0f });
		v[23] = Vertex(XMFLOAT3{ +w2, -h2, +d2 }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, XMFLOAT2{ 1.0f, 1.0f });

		meshData.mVertices.assign(&v[0], &v[24]);

		//
		// Create the indices.
		//

		std::uint32_t i[36U];

		// Fill in the front face index data
		i[0] = 0; i[1] = 1; i[2] = 2;
		i[3] = 0; i[4] = 2; i[5] = 3;

		// Fill in the back face index data
		i[6] = 4; i[7] = 5; i[8] = 6;
		i[9] = 4; i[10] = 6; i[11] = 7;

		// Fill in the top face index data
		i[12] = 8; i[13] = 9; i[14] = 10;
		i[15] = 8; i[16] = 10; i[17] = 11;

		// Fill in the bottom face index data
		i[18] = 12; i[19] = 13; i[20] = 14;
		i[21] = 12; i[22] = 14; i[23] = 15;

		// Fill in the left face index data
		i[24] = 16; i[25] = 17; i[26] = 18;
		i[27] = 16; i[28] = 18; i[29] = 19;

		// Fill in the right face index data
		i[30] = 20; i[31] = 21; i[32] = 22;
		i[33] = 20; i[34] = 22; i[35] = 23;

		meshData.mIndices32.assign(&i[0], &i[36]);

		// Put a cap on the number of subdivisions.
		const std::uint32_t clampedNumSubdivisions{ std::min<std::uint32_t>(numSubdivisions, 6U) };

		for (std::uint32_t j = 0U; j < clampedNumSubdivisions; ++j) {
			Subdivide(meshData);
		}
	}

	void GeometryGenerator::CreateSphere(
		const float radius, 
		const std::uint32_t sliceCount,
		const std::uint32_t stackCount, 
		MeshData& meshData) noexcept 
	{
		ASSERT(stackCount >= 2);
		ASSERT(sliceCount >= 1);

		//
		// Compute the vertices stating at the top pole and moving down the stacks.
		//

		// Poles: note that there will be texture coordinate distortion as there is
		// not a unique point on the texture map to assign to the pole when mapping
		// a rectangular texture onto a sphere.
		Vertex topVertex{ XMFLOAT3{ 0.0f, +radius, 0.0f }, XMFLOAT3{ 0.0f, +1.0f, 0.0f }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT2{ 0.0f, 0.0f }};
		Vertex bottomVertex{ XMFLOAT3{ 0.0f, -radius, 0.0f }, XMFLOAT3{ 0.0f, -1.0f, 0.0f }, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, XMFLOAT2{ 0.0f, 1.0f } };

		meshData.mVertices.push_back(topVertex);

		const float phiStep{ XM_PI / stackCount };
		const float thetaStep{ 2.0f * XM_PI / sliceCount };

		// Compute vertices for each stack ring (do not count the poles as rings).
		std::uint32_t count{ stackCount - 1U };
		for (std::uint32_t i = 1U; i <= count; ++i) {
			const float phi{ i * phiStep };

			// Vertices of ring.
			for (std::uint32_t j = 0U; j <= sliceCount; ++j) {
				const float theta{ j * thetaStep };

				Vertex v;

				// spherical to cartesian
				v.mPosition.x = radius * sinf(phi) * cosf(theta);
				v.mPosition.y = radius * cosf(phi);
				v.mPosition.z = radius * sinf(phi) * sinf(theta);

				// Partial derivative of P with respect to theta
				v.mTangent.x = -radius * sinf(phi) * sinf(theta);
				v.mTangent.y = 0.0f;
				v.mTangent.z = +radius * sinf(phi) * cosf(theta);

				const XMVECTOR T(XMLoadFloat3(&v.mTangent));
				XMStoreFloat3(&v.mTangent, XMVector3Normalize(T));

				const XMVECTOR p(XMLoadFloat3(&v.mPosition));
				XMStoreFloat3(&v.mNormal, XMVector3Normalize(p));

				v.mUV.x = theta / XM_2PI;
				v.mUV.y = phi / XM_PI;

				meshData.mVertices.push_back(v);
			}
		}

		meshData.mVertices.push_back(bottomVertex);

		//
		// Compute indices for top stack.  The top stack was written first to the vertex buffer
		// and connects the top pole to the first ring.
		//

		for (std::uint32_t i = 1U; i <= sliceCount; ++i) {
			meshData.mIndices32.push_back(0U);
			meshData.mIndices32.push_back(i + 1U);
			meshData.mIndices32.push_back(i);
		}

		//
		// Compute indices for inner stacks (not connected to poles).
		//

		// Offset the indices to the index of the first vertex in the first ring.
		// This is just skipping the top pole vertex.
		std::uint32_t baseIndex{ 1U };
		std::uint32_t ringVertexCount{ sliceCount + 1U };
		count = stackCount - 2U;
		for (std::uint32_t i = 0U; i < count; ++i) {
			for (std::uint32_t j = 0U; j < sliceCount; ++j) {
				meshData.mIndices32.push_back(baseIndex + i * ringVertexCount + j);
				meshData.mIndices32.push_back(baseIndex + i * ringVertexCount + j + 1U);
				meshData.mIndices32.push_back(baseIndex + (i + 1U) * ringVertexCount + j);

				meshData.mIndices32.push_back(baseIndex + (i + 1U) * ringVertexCount + j);
				meshData.mIndices32.push_back(baseIndex + i * ringVertexCount + j + 1U);
				meshData.mIndices32.push_back(baseIndex + (i + 1U) * ringVertexCount + j + 1U);
			}
		}

		//
		// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
		// and connects the bottom pole to the bottom ring.
		//

		// South pole vertex was added last.
		const std::uint32_t southPoleIndex{ static_cast<std::uint32_t>(meshData.mVertices.size()) - 1U };

		// Offset the indices to the index of the first vertex in the last ring.
		baseIndex = southPoleIndex - ringVertexCount;

		for (std::uint32_t i = 0U; i < sliceCount; ++i) {
			meshData.mIndices32.push_back(southPoleIndex);
			meshData.mIndices32.push_back(baseIndex + i);
			meshData.mIndices32.push_back(baseIndex + i + 1);
		}
	}
	
	void GeometryGenerator::CreateGeosphere(const float radius, const std::uint32_t numSubdivisions, MeshData& meshData) noexcept {
		// Put a cap on the number of subdivisions.
		const std::uint32_t clampedNumSubdivisions{ std::min<std::uint32_t>(numSubdivisions, 6U) };

		// Approximate a sphere by tessellating an icosahedron.

		const float X{ 0.525731f };
		const float Z{ 0.850651f };

		XMFLOAT3 pos[12] =
		{
			XMFLOAT3{ -X, 0.0f, Z },  XMFLOAT3{ X, 0.0f, Z },
			XMFLOAT3{ -X, 0.0f, -Z }, XMFLOAT3{ X, 0.0f, -Z },
			XMFLOAT3{ 0.0f, Z, X },   XMFLOAT3{ 0.0f, Z, -X },
			XMFLOAT3{ 0.0f, -Z, X },  XMFLOAT3{ 0.0f, -Z, -X },
			XMFLOAT3{ Z, X, 0.0f },   XMFLOAT3{ -Z, X, 0.0f },
			XMFLOAT3{ Z, -X, 0.0f },  XMFLOAT3{ -Z, -X, 0.0f }
		};

		std::uint32_t k[60U] =
		{
			1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
			1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
			3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
			10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
		};

		meshData.mVertices.resize(12U);
		meshData.mIndices32.assign(&k[0U], &k[60U]);

		for (std::uint32_t i = 0U; i < 12U; ++i) {
			meshData.mVertices[i].mPosition = pos[i];
		}

		for (std::uint32_t i = 0U; i < clampedNumSubdivisions; ++i) {
			Subdivide(meshData);
		}

		// Project mVertices onto sphere and scale.
		for (std::uint32_t i = 0U; i < meshData.mVertices.size(); ++i) {
			// Project onto unit sphere.
			const XMVECTOR n(XMVector3Normalize(XMLoadFloat3(&meshData.mVertices[i].mPosition)));

			// Project onto sphere.
			const XMVECTOR p(radius * n);

			XMStoreFloat3(&meshData.mVertices[i].mPosition, p);
			XMStoreFloat3(&meshData.mVertices[i].mNormal, n);

			// Derive texture coordinates from spherical coordinates.
			float theta{ atan2f(meshData.mVertices[i].mPosition.z, meshData.mVertices[i].mPosition.x) };

			// Put in [0, 2pi].
			if (theta < 0.0f) {
				theta += XM_2PI;
			}

			const float phi{ acosf(meshData.mVertices[i].mPosition.y / radius) };

			meshData.mVertices[i].mUV.x = theta / XM_2PI;
			meshData.mVertices[i].mUV.y = phi / XM_PI;

			// Partial derivative of P with respect to theta
			meshData.mVertices[i].mTangent.x = -radius * sinf(phi) * sinf(theta);
			meshData.mVertices[i].mTangent.y = 0.0f;
			meshData.mVertices[i].mTangent.z = +radius * sinf(phi) * cosf(theta);

			const XMVECTOR T(XMLoadFloat3(&meshData.mVertices[i].mTangent));
			XMStoreFloat3(&meshData.mVertices[i].mTangent, XMVector3Normalize(T));
		}
	}

	void CreateCylinder(
		const float bottomRadius, 
		const float topRadius, 
		const float height,
		const std::uint32_t sliceCount, 
		const std::uint32_t stackCount, 
		MeshData& meshData) noexcept 
	{
		//
		// Build Stacks.
		// 

		const float stackHeight{ height / stackCount };

		// Amount to increment radius as we move up each stack level from bottom to top.
		const float radiusStep{ (topRadius - bottomRadius) / stackCount };

		const std::uint32_t ringCount{ stackCount + 1U };

		// Compute mVertices for each stack ring starting at the bottom and moving up.
		for (std::uint32_t i = 0U; i < ringCount; ++i) {
			const float y{ -0.5f * height + i * stackHeight };
			const float r{ bottomRadius + i * radiusStep };

			// mVertices of ring
			const float dTheta{ 2.0f * XM_PI / sliceCount };
			for (std::uint32_t j = 0U; j <= sliceCount; ++j) {
				Vertex vertex;

				const float c{ cosf(j * dTheta) };
				const float s{ sinf(j * dTheta) };

				vertex.mPosition = XMFLOAT3{ r * c, y, r * s };

				vertex.mUV.x = static_cast<float>(j) / sliceCount;
				vertex.mUV.y = 1.0f - static_cast<float>(i) / stackCount;

				// Cylinder can be parameterized as follows, where we introduce v
				// parameter that goes in the same direction as the v tex-coord
				// so that the bitangent goes in the same direction as the v tex-coord.
				//   Let r0 be the bottom radius and let r1 be the top radius.
				//   y(v) = h - hv for v in [0,1].
				//   r(v) = r1 + (r0-r1)v
				//
				//   x(t, v) = r(v)*cos(t)
				//   y(t, v) = h - hv
				//   z(t, v) = r(v)*sin(t)
				// 
				//  dx/dt = -r(v)*sin(t)
				//  dy/dt = 0
				//  dz/dt = +r(v)*cos(t)
				//
				//  dx/dv = (r0-r1)*cos(t)
				//  dy/dv = -h
				//  dz/dv = (r0-r1)*sin(t)

				// This is unit length.
				vertex.mTangent = XMFLOAT3{ -s, 0.0f, c };

				const float dr{ bottomRadius - topRadius };
				const XMFLOAT3 bitangent{ dr * c, -height, dr * s };

				const XMVECTOR T(XMLoadFloat3(&vertex.mTangent));
				const XMVECTOR B(XMLoadFloat3(&bitangent));
				const XMVECTOR N(XMVector3Normalize(XMVector3Cross(T, B)));
				XMStoreFloat3(&vertex.mNormal, N);

				meshData.mVertices.push_back(vertex);
			}
		}

		// Add one because we duplicate the first and last vertex per ring
		// since the texture coordinates are different.
		const std::uint32_t ringVertexCount{ sliceCount + 1U };

		// Compute indices for each stack.
		for (std::uint32_t i = 0U; i < stackCount; ++i) {
			for (std::uint32_t j = 0U; j < sliceCount; ++j) {
				meshData.mIndices32.push_back(i * ringVertexCount + j);
				meshData.mIndices32.push_back((i + 1U) * ringVertexCount + j);
				meshData.mIndices32.push_back((i + 1U) * ringVertexCount + j + 1U);

				meshData.mIndices32.push_back(i * ringVertexCount + j);
				meshData.mIndices32.push_back((i + 1U) * ringVertexCount + j + 1U);
				meshData.mIndices32.push_back(i * ringVertexCount + j + 1U);
			}
		}

		BuildCylinderTopCap(topRadius, height, sliceCount, meshData);
		BuildCylinderBottomCap(bottomRadius, height, sliceCount, meshData);
	}

	void CreateGrid(
		const float width, 
		const float depth, 
		const std::uint32_t rows, 
		const std::uint32_t columns,
		MeshData& meshData) noexcept 
	{
		const std::uint32_t vertexCount{ rows * columns };
		const std::uint32_t faceCount{ (rows - 1U) * (columns - 1U) * 2U };

		//
		// Create the mVertices.
		//

		const float halfWidth{ 0.5f * width };
		const float halfDepth{ 0.5f * depth };

		const float dx{ width / (columns - 1U) };
		const float dz{ depth / (rows - 1U) };

		const float du{ 1.0f / (columns - 1U) };
		const float dv{ 1.0f / (rows - 1U) };

		meshData.mVertices.resize(vertexCount);
		for (std::uint32_t i = 0U; i < rows; ++i) {
			const float z{ halfDepth - i * dz };
			for (std::uint32_t j = 0U; j < columns; ++j) {
				const float x{ -halfWidth + j * dx };

				meshData.mVertices[i * columns + j].mPosition = XMFLOAT3{ x, 0.0f, z };
				meshData.mVertices[i * columns + j].mNormal = XMFLOAT3{ 0.0f, 1.0f, 0.0f };
				meshData.mVertices[i * columns + j].mTangent = XMFLOAT3{ 1.0f, 0.0f, 0.0f };

				// Stretch texture over grid.
				meshData.mVertices[i * columns + j].mUV.x = j * du;
				meshData.mVertices[i * columns + j].mUV.y = i * dv;
			}
		}

		//
		// Create the indices.
		//

		meshData.mIndices32.resize(faceCount * 3U); // 3 indices per face

													// Iterate over each quad and compute indices.
		std::uint32_t k{ 0U };
		const std::uint32_t count1{ rows - 1U };
		const std::uint32_t count2{ columns - 1U };
		for (std::uint32_t i = 0U; i < count1; ++i) {
			for (std::uint32_t j = 0U; j < count2; ++j) {
				meshData.mIndices32[k] = i * columns + j;
				meshData.mIndices32[k + 1U] = i * columns + j + 1U;
				meshData.mIndices32[k + 2U] = (i + 1U) * columns + j;

				meshData.mIndices32[k + 3U] = (i + 1U) * columns + j;
				meshData.mIndices32[k + 4U] = i * columns + j + 1U;
				meshData.mIndices32[k + 5U] = (i + 1U) * columns + j + 1U;

				k += 6U; // next quad
			}
		}
	}
}
