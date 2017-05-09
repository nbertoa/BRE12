#pragma once

#include <GeometryPass/GeometryPassCommandListRecorder.h>

namespace BRE {
class MaterialProperties;

// CommandListRecorders that does texture mapping
class TextureCommandListRecorder : public GeometryPassCommandListRecorder {
public:
    TextureCommandListRecorder() = default;
    ~TextureCommandListRecorder() = default;
    TextureCommandListRecorder(const TextureCommandListRecorder&) = delete;
    const TextureCommandListRecorder& operator=(const TextureCommandListRecorder&) = delete;
    TextureCommandListRecorder(TextureCommandListRecorder&&) = default;
    TextureCommandListRecorder& operator=(TextureCommandListRecorder&&) = default;

    static void InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats,
                                              const std::uint32_t geometryBufferCount) noexcept;

    // Preconditions:
    // - All containers must not be empty
    // - InitSharedPSOAndRootSignature() must be called first and once
    void Init(const std::vector<GeometryData>& geometryDataVector,
              const std::vector<MaterialProperties>& materialProperties,
              const std::vector<ID3D12Resource*>& diffuseTextures) noexcept;

    // Preconditions:
    // - Init() must be called first
    void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

    bool IsDataValid() const noexcept final override;

private:
    // Preconditions:
    // - All containers must not be empty
    void InitConstantBuffers(const std::vector<MaterialProperties>& materialProperties,
                             const std::vector<ID3D12Resource*>& diffuseTextures) noexcept;

    D3D12_GPU_DESCRIPTOR_HANDLE mBaseColorBufferGpuDescriptorsBegin;
};
}

