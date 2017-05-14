#pragma once

#include <GeometryPass/GeometryPassCommandListRecorder.h>

namespace BRE {
class MaterialProperties;

///
/// @brief Responsible to record command lists that implement normal mapping
///
class NormalCommandListRecorder : public GeometryPassCommandListRecorder {
public:
    NormalCommandListRecorder() = default;
    ~NormalCommandListRecorder() = default;
    NormalCommandListRecorder(const NormalCommandListRecorder&) = delete;
    const NormalCommandListRecorder& operator=(const NormalCommandListRecorder&) = delete;
    NormalCommandListRecorder(NormalCommandListRecorder&&) = default;
    NormalCommandListRecorder& operator=(NormalCommandListRecorder&&) = default;

    ///
    /// @brief Initializes pipeline state object and root signature
    /// @param geometryBufferFormats List of geometry buffers formats. It must be not nullptr
    /// @param geometryBufferCount Number of geometry buffers formats in @p geometryBufferFormats
    ///
    static void InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats,
                                              const std::uint32_t geometryBufferCount) noexcept;

    ///
    /// @brief Initializes the recorder
    ///
    /// InitSharedPSOAndRootSignature() must be called first
    ///
    /// @param geometryDataVector List of geometry data. Must not be empty
    /// @param materialProperties List of material properties. Must not be empty.
    /// @param diffuseTextures List of diffuse textures. Must not be empty.
    /// @param normalTextures List of normal textures. Must not be empty.
    ///
    void Init(const std::vector<GeometryData>& geometryDataVector,
              const std::vector<MaterialProperties>& materialProperties,
              const std::vector<ID3D12Resource*>& diffuseTextures,
              const std::vector<ID3D12Resource*>& normalTextures) noexcept;

    ///
    /// @brief Records and push command lists to CommandListExecutor
    ///
    /// Init() must be called first
    ///
    /// @param frameCBuffer Constant buffer per frame, for current frame
    ///
    void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept final override;

private:
    ///
    /// @brief Initializes the constant buffers
    /// @param materialProperties List of material properties. Must not be empty.
    /// @param diffuseTextures List of diffuse textures. Must not be empty.
    /// @param normalTextures List of normal textures. Must not be empty.
    ///
    void InitConstantBuffers(const std::vector<MaterialProperties>& materialProperties,
                             const std::vector<ID3D12Resource*>& diffuseTextures,
                             const std::vector<ID3D12Resource*>& normalTextures) noexcept;

    D3D12_GPU_DESCRIPTOR_HANDLE mBaseColorBufferGpuDescriptorsBegin;
    D3D12_GPU_DESCRIPTOR_HANDLE mNormalBufferGpuDescriptorsBegin;
};
}

