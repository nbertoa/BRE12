#pragma once

#include <cstdint>
#include <DirectXMath.h>
#include <vector>

struct aiMesh;
class Model;

class Mesh {
	friend class Model;

public:
	Mesh(const Mesh& rhs) = delete;
	Mesh& operator=(const Mesh& rhs) = delete;
	~Mesh();

	__forceinline const std::vector<DirectX::XMFLOAT3>& Vertices() const  noexcept { return mVertices; }
	__forceinline const std::vector<DirectX::XMFLOAT3>& Normals() const  noexcept { return mNormals; }
	__forceinline const std::vector<DirectX::XMFLOAT3>& Tangents() const  noexcept { return mTangents; }
	__forceinline const std::vector<DirectX::XMFLOAT3>& BiNormals() const  noexcept { return mBiNormals; }
	__forceinline const std::vector<DirectX::XMFLOAT3>& TextureCoordinates() const  noexcept { return mTextureCoordinates; }
	__forceinline const std::vector<DirectX::XMFLOAT4>& Colors() const  noexcept { return mColors; }
	__forceinline const std::uint32_t FaceCount() const  noexcept { return mFaceCount; }
	__forceinline const std::vector<std::uint32_t>& Indices() const  noexcept { return mIndices; }

private:
	Mesh(const aiMesh& mesh);

	std::vector<DirectX::XMFLOAT3> mVertices;
	std::vector<DirectX::XMFLOAT3> mNormals;
	std::vector<DirectX::XMFLOAT3> mTangents;
	std::vector<DirectX::XMFLOAT3> mBiNormals;
	std::vector<DirectX::XMFLOAT3> mTextureCoordinates;
	std::vector<DirectX::XMFLOAT4> mColors;
	std::uint32_t mFaceCount{ 0U };
	std::vector<std::uint32_t> mIndices;
};