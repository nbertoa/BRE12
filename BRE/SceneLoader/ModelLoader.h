#pragma once

#include <string>
#include <unordered_map>

namespace YAML {
class Node;
}

class Model;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;

class ModelLoader {
public:
    ModelLoader()
    {}
    ModelLoader(const ModelLoader&) = delete;
    const ModelLoader& operator=(const ModelLoader&) = delete;
    ModelLoader(ModelLoader&&) = delete;
    ModelLoader& operator=(ModelLoader&&) = delete;

    void LoadModels(const YAML::Node& rootNode,
                    ID3D12CommandAllocator& commandAllocator,
                    ID3D12GraphicsCommandList& commandList) noexcept;

    const Model& GetModel(const std::string& name) const noexcept;

private:
    std::unordered_map<std::string, Model*> mModelByName;
};