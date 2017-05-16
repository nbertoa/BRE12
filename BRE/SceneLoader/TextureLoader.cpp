#include "TextureLoader.h"

#include <d3d12.h>
#include <vector>
#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <CommandListExecutor\CommandListExecutor.h>
#include <ResourceManager\ResourceManager.h>
#include <Utils/DebugUtils.h>

namespace BRE {
void
TextureLoader::LoadTextures(const YAML::Node& rootNode,
                            ID3D12CommandAllocator& commandAllocator,
                            ID3D12GraphicsCommandList& commandList) noexcept
{
    BRE_ASSERT(rootNode.IsDefined());

    // Get the "textures" node. It is a map and its sintax is:
    // textures:
    //   textureName1: texturePath1
    //   textureName2: texturePath2
    //   textureName3: texturePath3
    const YAML::Node texturesNode = rootNode["textures"];
    BRE_ASSERT_MSG(texturesNode.IsDefined(), L"'textures' node is not found");
    BRE_ASSERT_MSG(texturesNode.IsMap(), L"'textures' node must be a map");

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadBuffers;
    BRE_CHECK_HR(commandList.Reset(&commandAllocator, nullptr));

    // Iterate "textureName: texturePath" pairs and create textures.
    std::string textureName;
    std::string texturePath;
    for (YAML::const_iterator it = texturesNode.begin(); it != texturesNode.end(); ++it) {
        textureName = it->first.as<std::string>();
        texturePath = it->second.as<std::string>();

        BRE_ASSERT_MSG(mTextureByName.find(textureName) == mTextureByName.end(), L"Texture name is not unique");

        uploadBuffers.resize(uploadBuffers.size() + 1);

        ID3D12Resource& texture = ResourceManager::LoadTextureFromFile(texturePath.c_str(),
                                                                       commandList,
                                                                       uploadBuffers.back(),
                                                                       nullptr);

        mTextureByName[textureName] = &texture;
    }

    commandList.Close();

    CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}

ID3D12Resource&
TextureLoader::GetTexture(const std::string& name) noexcept
{
    std::unordered_map<std::string, ID3D12Resource*>::iterator findIt = mTextureByName.find(name);
    BRE_ASSERT_MSG(findIt != mTextureByName.end(), L"Texture name not found");
    BRE_ASSERT(findIt->second != nullptr);

    return *findIt->second;
}
}

