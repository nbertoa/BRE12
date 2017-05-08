#pragma once

#include <GeometryPass/GeometryPassCommandListRecorder.h>

namespace BRE {
class MaterialProperties;

// CommandListRecorders that does color mapping
class ColorCommandListRecorder : public GeometryPassCommandListRecorder {
public:
    ColorCommandListRecorder() = default;
    ~ColorCommandListRecorder() = default;
    ColorCommandListRecorder(const ColorCommandListRecorder&) = delete;
    const ColorCommandListRecorder& operator=(const ColorCommandListRecorder&) = delete;
    ColorCommandListRecorder(ColorCommandListRecorder&&) = default;
    ColorCommandListRecorder& operator=(ColorCommandListRecorder&&) = default;

    static void InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept;

    // Preconditions:
    // - All containers must not be empty
    // - InitSharedPSOAndRootSignature() must be called first and once
    void Init(const std::vector<GeometryData>& geometryDataVector,
              const std::vector<MaterialProperties>& materialProperties) noexcept;

    // Preconditions:
    // - Init() must be called first
    void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

private:
    // Preconditions:
    // - All containers must not be empty
    void InitConstantBuffers(const std::vector<MaterialProperties>& materialProperties) noexcept;
};
}
