#pragma once

#include <GeometryPass/GeometryPassCommandListRecorder.h>

namespace BRE {
class MaterialProperties;

// CommandListRecorders that does color mapping + normal mapping + height mapping
class ColorHeightCommandListRecorder : public GeometryPassCommandListRecorder {
public:
    ColorHeightCommandListRecorder() = default;
    ~ColorHeightCommandListRecorder() = default;
    ColorHeightCommandListRecorder(const ColorHeightCommandListRecorder&) = delete;
    const ColorHeightCommandListRecorder& operator=(const ColorHeightCommandListRecorder&) = delete;
    ColorHeightCommandListRecorder(ColorHeightCommandListRecorder&&) = default;
    ColorHeightCommandListRecorder& operator=(ColorHeightCommandListRecorder&&) = default;

    static void InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats, 
    const std::uint32_t geometryBufferCount) noexcept;

    // Preconditions:
    // - All containers must not be empty
    // - InitSharedPSOAndRootSignature() must be called first and once
    void Init(const std::vector<GeometryData>& geometryDataVector,
              const std::vector<MaterialProperties>& materialProperties,
              const std::vector<ID3D12Resource*>& normalTextures,
              const std::vector<ID3D12Resource*>& heightTextures) noexcept;

    // Preconditions:
    // - Init() must be called first
    void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

    bool IsDataValid() const noexcept final override;

private:
    // Preconditions:
    // - All containers must not be empty
    void InitConstantBuffers(const std::vector<MaterialProperties>& materialProperties,
                             const std::vector<ID3D12Resource*>& normalTextures,
                             const std::vector<ID3D12Resource*>& heightTextures) noexcept;

    D3D12_GPU_DESCRIPTOR_HANDLE mNormalBufferGpuDescriptorsBegin;
    D3D12_GPU_DESCRIPTOR_HANDLE mHeightBufferGpuDescriptorsBegin;
};
}