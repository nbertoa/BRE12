#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstdint>
#include <iostream>
#include <sstream>

#include <ResourceManager/ResourceManager.h>
#include <ModelManager/Mesh.h>
#include <Utils/DebugUtils.h>

Model::Model(const char* filename) {
	ASSERT(filename != nullptr);
	Assimp::Importer importer;
	const std::uint32_t flags{ aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded };
	const aiScene* scene{ importer.ReadFile(filename, flags) };
	if (scene == nullptr) {
		const std::string errorMsg{ importer.GetErrorString() };
		const std::wstring msg = StringUtils::AnsiToWString(errorMsg);
		MessageBox(0, msg.c_str(), 0, 0);
		ASSERT(scene != nullptr);
	}

	ASSERT(scene->HasMeshes());

	for (std::uint32_t i = 0U; i < scene->mNumMeshes; ++i) {
		aiMesh* mesh{ scene->mMeshes[i] };
		ASSERT(mesh != nullptr);
		mMeshes.push_back(new Mesh(*mesh));
	}
}

Model::~Model() {
	for (Mesh* mesh : mMeshes) {
		delete mesh;
	}
}