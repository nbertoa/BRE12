#pragma once

#include <GeometryPass/GeometryCommandListRecorder.h>

namespace BRE {
class MaterialProperties;

///
/// @brief Responsible to record command lists that implement color mapping
///
class ColorMappingCommandListRecorder : public GeometryCommandListRecorder {
public:
    ColorMappingCommandListRecorder() = default;
    ~ColorMappingCommandListRecorder() = default;
    ColorMappingCommandListRecorder(const ColorMappingCommandListRecorder&) = delete;
    const ColorMappingCommandListRecorder& operator=(const ColorMappingCommandListRecorder&) = delete;
    ColorMappingCommandListRecorder(ColorMappingCommandListRecorder&&) = default;
    ColorMappingCommandListRecorder& operator=(ColorMappingCommandListRecorder&&) = default;

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
    ///
    void Init(const std::vector<GeometryData>& geometryDataVector,
              const std::vector<MaterialProperties>& materialProperties) noexcept;

    ///
    /// @brief Records and push command lists to CommandListExecutor
    ///
    /// Init() must be called first
    ///
    /// @param frameCBuffer Constant buffer per frame, for current frame
    /// @return The number of pushed command lists
    ///
    std::uint32_t RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

private:
    ///
    /// @brief Initializes the constant buffers
    /// @param materialProperties List of material properties. Must not be empty.
    ///
    void InitConstantBuffers(const std::vector<MaterialProperties>& materialProperties) noexcept;
};
}