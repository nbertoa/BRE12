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
    ///
    /// @brief Load models
    /// @param modelsNode YAML Node representing the "models" field. It must be a map.
    /// @param commandAllocator Command allocator for the command list to load textures
    /// @param commandList Command list to load the textures
    /// @param uploadVertexBuffer Upload buffer to upload the buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadVertexBuffer after it knows the copy has been executed.
    /// @param uploadIndexBuffers Upload buffer to upload the buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadIndexBuffers after it knows the copy has been executed.
    ///
    void LoadModels(const YAML::Node& modelsNode,
                    ID3D12CommandAllocator& commandAllocator,
                    ID3D12GraphicsCommandList& commandList,
                    std::vector<ID3D12Resource*>& uploadVertexBuffers,
                    std::vector<ID3D12Resource*>& uploadIndexBuffers) noexcept;

    std::unordered_map<std::string, Model*> mModelByName;
};
}