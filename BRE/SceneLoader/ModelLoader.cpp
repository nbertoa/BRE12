#include "ModelLoader.h"

#include <d3d12.h>
#include <vector>
#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <CommandListExecutor\CommandListExecutor.h>
#include <ModelManager\Model.h>
#include <ModelManager\ModelManager.h>
#include <Utils/DebugUtils.h>

void
ModelLoader::LoadModels(const YAML::Node& rootNode,
                        ID3D12CommandAllocator& commandAllocator,
                        ID3D12GraphicsCommandList& commandList) noexcept
{
    ASSERT(rootNode.IsDefined());

    // Get the "models" node. It is a map and its sintax is:
    // models:
    //   modelName1: modelPath1
    //   modelName2: modelPath2
    //   modelName3: modelPath3
    const YAML::Node modelsNode = rootNode["models"];
    ASSERT_MSG(modelsNode.IsDefined(), L"'models' node not found");
    ASSERT_MSG(modelsNode.IsMap(), L"'models' node must be a map");

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadVertexBuffers;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadIndexBuffers;
    CHECK_HR(commandList.Reset(&commandAllocator, nullptr));

    // Iterate "modelName: modelPath" pairs and create models.
    std::string modelName;
    std::string modelPath;
    for (YAML::const_iterator it = modelsNode.begin(); it != modelsNode.end(); ++it) {
        modelName = it->first.as<std::string>();
        modelPath = it->second.as<std::string>();

        ASSERT_MSG(mModelByName.find(modelName) == mModelByName.end(), L"Model name must be unique");

        uploadVertexBuffers.resize(uploadVertexBuffers.size() + 1);
        uploadIndexBuffers.resize(uploadIndexBuffers.size() + 1);

        Model& model = ModelManager::LoadModel(modelPath.c_str(),
                                               commandList,
                                               uploadVertexBuffers.back(),
                                               uploadIndexBuffers.back());

        mModelByName[modelName] = &model;
    }

    commandList.Close();

    CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}

const Model& ModelLoader::GetModel(const std::string& name) const noexcept
{
    std::unordered_map<std::string, Model*>::const_iterator findIt = mModelByName.find(name);
    ASSERT_MSG(findIt != mModelByName.end(), L"Model name not found");
    ASSERT(findIt->second != nullptr);

    return *findIt->second;
}