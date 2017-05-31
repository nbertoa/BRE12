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
    ///
    /// @brief Load textures
    /// @param texturesNode YAML Node representing the "textures" field. It must be a map.
    /// @param commandAllocator Command allocator for the command list to load textures
    /// @param commandList Command list to load the textures
    /// @param uploadBuffer Upload buffer to upload the texture content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadBuffer after it knows the copy has been executed.
    ///
    void LoadTextures(const YAML::Node& texturesNode,
                      ID3D12CommandAllocator& commandAllocator,
                      ID3D12GraphicsCommandList& commandList,
                      std::vector<ID3D12Resource*>& uploadBuffers) noexcept;


    std::unordered_map<std::string, ID3D12Resource*> mTextureByName;
};
}