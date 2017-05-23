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

namespace BRE {
void
ModelLoader::LoadModels(const YAML::Node& rootNode,
                        ID3D12CommandAllocator& commandAllocator,
                        ID3D12GraphicsCommandList& commandList) noexcept
{
    BRE_ASSERT(rootNode.IsDefined());

    // Get the "models" node. It is a map and its sintax is:
    // models:
    //   modelName1: modelPath1
    //   modelName2: modelPath2
    //   modelName3: modelPath3
    const YAML::Node modelsNode = rootNode["models"];
    BRE_ASSERT_MSG(modelsNode.IsDefined(), L"'models' node not found");
    BRE_ASSERT_MSG(modelsNode.IsMap(), L"'models' node must be a map");

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadVertexBuffers;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadIndexBuffers;
    BRE_CHECK_HR(commandList.Reset(&commandAllocator, nullptr));

    LoadModels(modelsNode,
               commandAllocator,
               commandList,
               uploadVertexBuffers,
               uploadIndexBuffers);

    commandList.Close();

    CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}

const Model& ModelLoader::GetModel(const std::string& name) const noexcept
{
    std::unordered_map<std::string, Model*>::const_iterator findIt = mModelByName.find(name);
    BRE_ASSERT_MSG(findIt != mModelByName.end(), L"Model name not found");
    BRE_ASSERT(findIt->second != nullptr);

    return *findIt->second;
}

void
ModelLoader::LoadModels(const YAML::Node& modelsNode,
                        ID3D12CommandAllocator& commandAllocator,
                        ID3D12GraphicsCommandList& commandList,
                        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& uploadVertexBuffers,
                        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& uploadIndexBuffers) noexcept
{
    BRE_ASSERT_MSG(modelsNode.IsMap(), L"'models' node must be a map");

    std::string name;
    std::string path;
    for (YAML::const_iterator it = modelsNode.begin(); it != modelsNode.end(); ++it) {
        name = it->first.as<std::string>();
        path = it->second.as<std::string>();

        // If name is "reference", then path must be a yaml file that specifies "models"
        if (name == "reference") {
            const YAML::Node referenceRootNode = YAML::LoadFile(path);
            BRE_ASSERT_MSG(referenceRootNode.IsDefined(), L"Failed to open yaml file");
            const YAML::Node referenceModelsNode = referenceRootNode["models"];
            LoadModels(referenceModelsNode,
                       commandAllocator,
                       commandList,
                       uploadVertexBuffers,
                       uploadIndexBuffers);
        } else {
            BRE_ASSERT_MSG(mModelByName.find(name) == mModelByName.end(), L"Model name must be unique");

            uploadVertexBuffers.resize(uploadVertexBuffers.size() + 1);
            uploadIndexBuffers.resize(uploadIndexBuffers.size() + 1);

            Model& model = ModelManager::LoadModel(path.c_str(),
                                                   commandList,
                                                   uploadVertexBuffers.back(),
                                                   uploadIndexBuffers.back());

            mModelByName[name] = &model;
        }
    }
}
}