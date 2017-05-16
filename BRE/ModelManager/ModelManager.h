#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb\concurrent_unordered_set.h>

#include <ModelManager/Model.h>

namespace BRE {
///
/// @brief Responsible to create models or built-in geometry (box, sphere, etc)
///
class ModelManager {
public:
    ModelManager() = delete;
    ~ModelManager() = delete;
    ModelManager(const ModelManager&) = delete;
    const ModelManager& operator=(const ModelManager&) = delete;
    ModelManager(ModelManager&&) = delete;
    ModelManager& operator=(ModelManager&&) = delete;

    ///
    /// @brief Releases all models
    ///
    static void Clear() noexcept;

    ///
    /// @brief Load model
    /// @param modelFilename Model filename. Must be not nullptr
    /// @param commandList Command list used to upload buffers content to GPU.
    /// It must be executed after this function call to upload buffers content to GPU.
    /// It must be in recording state before calling this method.
    /// @param uploadVertexBuffer Upload buffer to upload the vertex buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadVertexBuffer after it knows the copy has been executed.
    /// @param uploadIndexBuffer Upload buffer to upload the index buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadIndexBuffer after it knows the copy has been executed.
    /// @return Model
    ///
    static Model& LoadModel(const char* modelFilename,
                            ID3D12GraphicsCommandList& commandList,
                            Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                            Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

    ///
    /// @brief Create a box centered at the origin
    /// @param width Width
    /// @param height Height
    /// @param depth Depth
    /// @param numSubdivisions Number of subdivisions. This controls tessellation.
    /// @param commandList Command list used to upload buffers content to GPU.
    /// It must be executed after this function call to upload buffers content to GPU.
    /// It must be in recording state before calling this method.
    /// @param uploadVertexBuffer Upload buffer to upload the vertex buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadVertexBuffer after it knows the copy has been executed.
    /// @param uploadIndexBuffer Upload buffer to upload the index buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadIndexBuffer after it knows the copy has been executed.
    /// @return Model
    ///
    static Model& CreateBox(const float width,
                            const float height,
                            const float depth,
                            const std::uint32_t numSubdivisions,
                            ID3D12GraphicsCommandList& commandList,
                            Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                            Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

    ///
    /// @brief Create a sphere centered at the origin
    /// @param radius Radius
    /// @param sliceCount Slice count. This controls tessellation.
    /// @param stackCount Stack count. This controls tessellation.
    /// @param commandList Command list used to upload buffers content to GPU.
    /// It must be executed after this function call to upload buffers content to GPU.
    /// It must be in recording state before calling this method.
    /// @param uploadVertexBuffer Upload buffer to upload the vertex buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadVertexBuffer after it knows the copy has been executed.
    /// @param uploadIndexBuffer Upload buffer to upload the index buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadIndexBuffer after it knows the copy has been executed.
    /// @return Model
    ///
    static Model& CreateSphere(const float radius,
                               const std::uint32_t sliceCount,
                               const std::uint32_t stackCount,
                               ID3D12GraphicsCommandList& commandList,
                               Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                               Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

    ///
    /// @brief Create a geosphere centered at the origin
    /// @param radius Radius
    /// @param numSubdivisions Number of subdivisions. This controls tessellation.
    /// @param commandList Command list used to upload buffers content to GPU.
    /// It must be executed after this function call to upload buffers content to GPU.
    /// It must be in recording state before calling this method.
    /// @param uploadVertexBuffer Upload buffer to upload the vertex buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadVertexBuffer after it knows the copy has been executed.
    /// @param uploadIndexBuffer Upload buffer to upload the index buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadIndexBuffer after it knows the copy has been executed.
    /// @return Model
    ///
    static Model& CreateGeosphere(const float radius,
                                  const std::uint32_t numSubdivisions,
                                  ID3D12GraphicsCommandList& commandList,
                                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

    ///
    /// @brief Create a cylinder centered at the origin
    /// @param bottomRadius Bottom radius
    /// @param topRadius Top radius
    /// @param height Height
    /// @param sliceCount Slice count. This controls tessellation.
    /// @param stackCount Stack count. This controls tessellation.
    /// @param commandList Command list used to upload buffers content to GPU.
    /// It must be executed after this function call to upload buffers content to GPU.
    /// It must be in recording state before calling this method.
    /// @param uploadVertexBuffer Upload buffer to upload the vertex buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadVertexBuffer after it knows the copy has been executed.
    /// @param uploadIndexBuffer Upload buffer to upload the index buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadIndexBuffer after it knows the copy has been executed.
    /// @return Model
    /// 
    static Model& CreateCylinder(const float bottomRadius,
                                 const float topRadius,
                                 const float height,
                                 const std::uint32_t sliceCount,
                                 const std::uint32_t stackCount,
                                 ID3D12GraphicsCommandList& commandList,
                                 Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                                 Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

    ///
    /// @brief Create a rows X columns grid in the xz-plane centered at the origin
    /// @param width Width
    /// @param depth Depth
    /// @param rows Grid rows
    /// @param columns Grid columns
    /// @param commandList Command list used to upload buffers content to GPU.
    /// It must be executed after this function call to upload buffers content to GPU.
    /// It must be in recording state before calling this method.
    /// @param uploadVertexBuffer Upload buffer to upload the vertex buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadVertexBuffer after it knows the copy has been executed.
    /// @param uploadIndexBuffer Upload buffer to upload the index buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadIndexBuffer after it knows the copy has been executed.
    /// @return Model
    ///
    static Model& CreateGrid(const float width,
                             const float depth,
                             const std::uint32_t rows,
                             const std::uint32_t columns,
                             ID3D12GraphicsCommandList& commandList,
                             Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                             Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

private:
    static tbb::concurrent_unordered_set<Model*> mModels;

    static std::mutex mMutex;
};

}

