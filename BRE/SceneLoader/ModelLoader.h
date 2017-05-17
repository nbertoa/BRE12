#pragma once

#include <string>
#include <unordered_map>

namespace YAML {
class Node;
}

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;

namespace BRE {
class Model;

///
/// @brief Responsible to load from scene file the models configurations
///
class ModelLoader {
public:
    ModelLoader()
    {}
    ModelLoader(const ModelLoader&) = delete;
    const ModelLoader& operator=(const ModelLoader&) = delete;
    ModelLoader(ModelLoader&&) = delete;
    ModelLoader& operator=(ModelLoader&&) = delete;

    ///
    /// @brief Load models
    /// @param rootNode Scene YAML file root node
    /// @param commandAllocator Command allocator for the command list to load models
    /// @param commandList Command list to load models
    ///
    void LoadModels(const YAML::Node& rootNode,
                    ID3D12CommandAllocator& commandAllocator,
                    ID3D12GraphicsCommandList& commandList) noexcept;

    ///
    /// @brief Get model
    /// @param name Model name
    /// @return Model
    ///
    const Model& GetModel(const std::string& name) const noexcept;

private:
    std::unordered_map<std::string, Model*> mModelByName;
};
}