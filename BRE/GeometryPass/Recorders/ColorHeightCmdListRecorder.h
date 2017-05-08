#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

class MaterialProperties;

// CommandListRecorders that does color mapping + normal mapping + height mapping
class ColorHeightCmdListRecorder : public GeometryPassCmdListRecorder {
public:
    ColorHeightCmdListRecorder() = default;
    ~ColorHeightCmdListRecorder() = default;
    ColorHeightCmdListRecorder(const ColorHeightCmdListRecorder&) = delete;
    const ColorHeightCmdListRecorder& operator=(const ColorHeightCmdListRecorder&) = delete;
    ColorHeightCmdListRecorder(ColorHeightCmdListRecorder&&) = default;
    ColorHeightCmdListRecorder& operator=(ColorHeightCmdListRecorder&&) = default;

    static void InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept;

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