#pragma once

#include <vector>

class Mesh;

class Model {
public:
	Model(const char* filename);
	Model(const Model& rhs) = delete;
	Model& operator=(const Model& rhs) = delete;
	~Model();

	__forceinline bool HasMeshes() const noexcept { return (mMeshes.size() > 0UL); }
	__forceinline const std::vector<Mesh*>& Meshes() const noexcept { return mMeshes; }

private:
	std::vector<Mesh*> mMeshes;
};
