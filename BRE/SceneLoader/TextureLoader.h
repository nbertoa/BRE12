#pragma once

#include <string>
#include <unordered_map>

namespace YAML {
class Node;
}

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

namespace BRE {
///
/// @brief Responsible to load from scene file the textures 
///
class TextureLoader {
public:
    TextureLoader()
    {}
    TextureLoader(const TextureLoader&) = delete;
    const TextureLoader& operator=(const TextureLoader&) = delete;
    TextureLoader(TextureLoader&&) = delete;
    TextureLoader& operator=(TextureLoader&&) = delete;

    ///
    /// @brief Load textures
    /// @param rootNode Scene YAML file root node
    /// @param commandAllocator Command allocator for the command list to load textures
    /// @param commandList Command list to load the textures
    ///
    void LoadTextures(const YAML::Node& rootNode,
                      ID3D12CommandAllocator& commandAllocator,
                      ID3D12GraphicsCommandList& commandList) noexcept;

    ///
    /// @brief Get texture
    /// @param name Texture name
    /// @return Texture
    ///
    ID3D12Resource& GetTexture(const std::string& name) noexcept;

private:
    std::unordered_map<std::string, ID3D12Resource*> mTextureByName;
};
}