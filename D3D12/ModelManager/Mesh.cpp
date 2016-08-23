#include "Mesh.h"

#include <assimp/scene.h>

#include <Utils/DebugUtils.h>

using namespace DirectX;

namespace {
	void CalculateTangentArray(Mesh& mesh, std::vector<XMFLOAT3>& tangents) {
		const std::size_t vertexCount{ mesh.Vertices().size() };
		const std::size_t triangleCount{ mesh.FaceCount() };
		const std::vector<XMFLOAT3>& texCoords(mesh.TextureCoordinates());
		tangents.resize(vertexCount);
		XMFLOAT3 *tan1 = new XMFLOAT3[vertexCount * 2U];
		XMFLOAT3 *tan2 = tan1 + vertexCount;
		ZeroMemory(tan1, vertexCount * sizeof(XMFLOAT3) * 2U);
		std::size_t baseIndex{ 0UL };
		for (long a = 0; a < triangleCount; ++a) {
			const std::size_t i1{ mesh.Indices()[baseIndex] };
			const std::size_t i2{ mesh.Indices()[baseIndex + 1U] };
			const std::size_t i3{ mesh.Indices()[baseIndex + 2U] };

			const XMFLOAT3& v1 = mesh.Vertices()[i1];
			const XMFLOAT3& v2 = mesh.Vertices()[i2];
			const XMFLOAT3& v3 = mesh.Vertices()[i3];

			const XMFLOAT3& w1 = texCoords[i1];
			const XMFLOAT3& w2 = texCoords[i2];
			const XMFLOAT3& w3 = texCoords[i3];

			const float x1{ v2.x - v1.x };
			const float x2{ v3.x - v1.x };
			const float y1{ v2.y - v1.y };
			const float y2{ v3.y - v1.y };
			const float z1{ v2.z - v1.z };
			const float z2{ v3.z - v1.z };

			const float s1{ w2.x - w1.x };
			const float s2{ w3.x - w1.x };
			const float t1{ w2.y - w1.y };
			const float t2{ w3.y - w1.y };

			const float r{ 1.0f / (s1 * t2 - s2 * t1) };
			const XMFLOAT3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
			const XMFLOAT3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

			XMStoreFloat3(&tan1[i1], XMVectorAdd(XMLoadFloat3(&tan1[i1]), XMLoadFloat3(&sdir)));
			XMStoreFloat3(&tan1[i2], XMVectorAdd(XMLoadFloat3(&tan1[i2]), XMLoadFloat3(&sdir)));
			XMStoreFloat3(&tan1[i3], XMVectorAdd(XMLoadFloat3(&tan1[i3]), XMLoadFloat3(&sdir)));

			XMStoreFloat3(&tan2[i1], XMVectorAdd(XMLoadFloat3(&tan2[i1]), XMLoadFloat3(&tdir)));
			XMStoreFloat3(&tan2[i2], XMVectorAdd(XMLoadFloat3(&tan2[i2]), XMLoadFloat3(&tdir)));
			XMStoreFloat3(&tan2[i3], XMVectorAdd(XMLoadFloat3(&tan2[i3]), XMLoadFloat3(&tdir)));

			baseIndex += 3;
		}
	}
}

Mesh::Mesh(const aiMesh& mesh) {

	// Vertices
	ASSERT(mesh.mNumVertices > 0U);
	mVertices.reserve(mesh.mNumVertices);
	for (std::uint32_t i = 0U; i < mesh.mNumVertices; ++i) {
		mVertices.push_back(XMFLOAT3((const float*)&mesh.mVertices[i]));
	}

	// Normals
	ASSERT(mesh.HasNormals());
	mNormals.reserve(mesh.mNumVertices);
	for (std::uint32_t i = 0U; i < mesh.mNumVertices; ++i) {
		mNormals.push_back(XMFLOAT3((const float*)&mesh.mNormals[i]));
	}


	// Texture Coordinates
	if (mesh.HasTextureCoords(0U)) {
		ASSERT(mesh.GetNumUVChannels() == 1U);
		mTextureCoordinates.reserve(mesh.mNumVertices);
		const aiVector3D* aiTextureCoordinates{ mesh.mTextureCoords[0U] };
		ASSERT(aiTextureCoordinates != nullptr);
		for (std::uint32_t j = 0U; j < mesh.mNumVertices; j++) {
			mTextureCoordinates.push_back(XMFLOAT3((const float*)&aiTextureCoordinates[j]));
		}
	}

	// Colors
	if (mesh.HasVertexColors(0U)) {
		ASSERT(mesh.GetNumColorChannels() == 1U);
		const aiColor4D* aiColors{ mesh.mColors[0U] };
		ASSERT(aiColors != nullptr);
		for (std::uint32_t j = 0U; j < mesh.mNumVertices; j++) {
			mColors.push_back(XMFLOAT4((const float*)&aiColors[j]));
		}
	}

	// We only allow triangles
	ASSERT(mesh.HasFaces());
	mFaceCount = mesh.mNumFaces;
	for (std::uint32_t i = 0U; i < mFaceCount; ++i) {
		const aiFace* face = &mesh.mFaces[i];
		ASSERT(face != nullptr);
		ASSERT(face->mNumIndices == 3U);
		mIndices.push_back(face->mIndices[0U]);
		mIndices.push_back(face->mIndices[1U]);
		mIndices.push_back(face->mIndices[2U]);
	}

	// Tangents and Binormals
	mTangents.reserve(mesh.mNumVertices);
	mBiNormals.reserve(mesh.mNumVertices);
	if (mesh.HasTangentsAndBitangents()) {
		for (std::uint32_t i = 0U; i < mesh.mNumVertices; ++i) {
			mTangents.push_back(XMFLOAT3((const float*)&mesh.mTangents[i]));
			mBiNormals.push_back(XMFLOAT3((const float*)&mesh.mBitangents[i]));
		}
	}
	else {
		CalculateTangentArray(*this, mTangents);
	}

}

Mesh::~Mesh() { }