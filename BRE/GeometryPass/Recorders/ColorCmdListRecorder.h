#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

class MaterialProperties;

// CommandListRecorders that does color mapping
class ColorCmdListRecorder : public GeometryPassCmdListRecorder {
public:
    ColorCmdListRecorder() = default;
    ~ColorCmdListRecorder() = default;
    ColorCmdListRecorder(const ColorCmdListRecorder&) = delete;
    const ColorCmdListRecorder& operator=(const ColorCmdListRecorder&) = delete;
    ColorCmdListRecorder(ColorCmdListRecorder&&) = default;
    ColorCmdListRecorder& operator=(ColorCmdListRecorder&&) = default;

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