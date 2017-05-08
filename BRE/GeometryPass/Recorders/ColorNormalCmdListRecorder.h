#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

namespace BRE {
class MaterialProperties;

// CommandListRecorders that does color mapping + normal mapping
class ColorNormalCmdListRecorder : public GeometryPassCmdListRecorder {
public:
    ColorNormalCmdListRecorder() = default;
    ~ColorNormalCmdListRecorder() = default;
    ColorNormalCmdListRecorder(const ColorNormalCmdListRecorder&) = delete;
    const ColorNormalCmdListRecorder& operator=(const ColorNormalCmdListRecorder&) = delete;
    ColorNormalCmdListRecorder(ColorNormalCmdListRecorder&&) = default;
    ColorNormalCmdListRecorder& operator=(ColorNormalCmdListRecorder&&) = default;

    static void InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept;

    // Preconditions:
    // - All containers must not be empty
    // - InitSharedPSOAndRootSignature() must be called first and once
    void Init(const std::vector<GeometryData>& geometryDataVector,
              const std::vector<MaterialProperties>& materialProperties,
              const std::vector<ID3D12Resource*>& normalTextures) noexcept;

    // Preconditions:
    // - Init() must be called first
    void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

    bool IsDataValid() const noexcept final override;

private:
    // Preconditions:
    // - All containers must not be empty
    void InitConstantBuffers(const std::vector<MaterialProperties>& materialProperties,
                             const std::vector<ID3D12Resource*>& normalTextures) noexcept;

    D3D12_GPU_DESCRIPTOR_HANDLE mNormalBufferGpuDescriptorsBegin;
};
}

