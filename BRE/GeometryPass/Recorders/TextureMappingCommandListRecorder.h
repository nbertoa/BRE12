#pragma once

#include <GeometryPass/GeometryCommandListRecorder.h>

namespace BRE {
class MaterialProperties;

///
/// @brief Responsible to record command lists that implement texture mapping
///
class TextureMappingCommandListRecorder : public GeometryCommandListRecorder {
public:
    TextureMappingCommandListRecorder() = default;
    ~TextureMappingCommandListRecorder() = default;
    TextureMappingCommandListRecorder(const TextureMappingCommandListRecorder&) = delete;
    const TextureMappingCommandListRecorder& operator=(const TextureMappingCommandListRecorder&) = delete;
    TextureMappingCommandListRecorder(TextureMappingCommandListRecorder&&) = default;
    TextureMappingCommandListRecorder& operator=(TextureMappingCommandListRecorder&&) = default;

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
    ///
    void Init(const std::vector<GeometryData>& geometryDataVector,
              const std::vector<MaterialProperties>& materialProperties,
              const std::vector<ID3D12Resource*>& diffuseTextures) noexcept;

    ///
    /// @brief Records and push command lists to CommandListExecutor
    ///
    /// Init() must be called first
    ///
    /// @param frameCBuffer Constant buffer per frame, for current frame
    /// @return The number of pushed command lists
    ///
    std::uint32_t RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept final override;

private:
    ///
    /// @brief Initializes the constant buffers and views
    /// @param materialProperties List of material properties. Must not be empty.
    /// @param diffuseTextures List of diffuse textures. Must not be empty.
    ///
    void InitCBuffersAndViews(const std::vector<MaterialProperties>& materialProperties,
                              const std::vector<ID3D12Resource*>& diffuseTextures) noexcept;

    D3D12_GPU_DESCRIPTOR_HANDLE mBaseColorTextureRenderTargetViewsBegin;
};
}