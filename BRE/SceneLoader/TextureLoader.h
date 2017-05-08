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
class TextureLoader {
public:
    TextureLoader()
    {}
    TextureLoader(const TextureLoader&) = delete;
    const TextureLoader& operator=(const TextureLoader&) = delete;
    TextureLoader(TextureLoader&&) = delete;
    TextureLoader& operator=(TextureLoader&&) = delete;

    void LoadTextures(const YAML::Node& rootNode,
                      ID3D12CommandAllocator& commandAllocator,
                      ID3D12GraphicsCommandList& commandList) noexcept;

    ID3D12Resource& GetTexture(const std::string& name) noexcept;

private:
    std::unordered_map<std::string, ID3D12Resource*> mTextureByName;
};
}

