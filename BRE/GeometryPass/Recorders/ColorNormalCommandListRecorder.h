#pragma once

#include <GeometryPass/GeometryPassCommandListRecorder.h>

namespace BRE {
class MaterialProperties;

// CommandListRecorders that does color mapping + normal mapping
class ColorNormalCommandListRecorder : public GeometryPassCommandListRecorder {
public:
    ColorNormalCommandListRecorder() = default;
    ~ColorNormalCommandListRecorder() = default;
    ColorNormalCommandListRecorder(const ColorNormalCommandListRecorder&) = delete;
    const ColorNormalCommandListRecorder& operator=(const ColorNormalCommandListRecorder&) = delete;
    ColorNormalCommandListRecorder(ColorNormalCommandListRecorder&&) = default;
    ColorNormalCommandListRecorder& operator=(ColorNormalCommandListRecorder&&) = default;

    static void InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats,
                                              const std::uint32_t geometryBufferCount) noexcept;

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

