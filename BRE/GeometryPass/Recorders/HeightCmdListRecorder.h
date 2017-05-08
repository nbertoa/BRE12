#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

class MaterialProperties;

// CommandListRecorders that does texture mapping + normal mapping + height mapping
class HeightCmdListRecorder : public GeometryPassCmdListRecorder {
public:
	HeightCmdListRecorder() = default;
	~HeightCmdListRecorder() = default;
	HeightCmdListRecorder(const HeightCmdListRecorder&) = delete;
	const HeightCmdListRecorder& operator=(const HeightCmdListRecorder&) = delete;
	HeightCmdListRecorder(HeightCmdListRecorder&&) = default;
	HeightCmdListRecorder& operator=(HeightCmdListRecorder&&) = default;

	static void InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept;

	// Preconditions:
	// - All containers must not be empty
	// - InitSharedPSOAndRootSignature() must be called first and once
	void Init(
		const std::vector<GeometryData>& geometryDataVector,
		const std::vector<MaterialProperties>& materialProperties,
		const std::vector<ID3D12Resource*>& diffuseTextures,
		const std::vector<ID3D12Resource*>& normalTextures,
		const std::vector<ID3D12Resource*>& heightTextures) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

	bool IsDataValid() const noexcept final override;

private:
	// Preconditions:
	// - All containers must not be empty
	void InitConstantBuffers(
		const std::vector<MaterialProperties>& materialProperties,
		const std::vector<ID3D12Resource*>& diffuseTextures,
		const std::vector<ID3D12Resource*>& normalTextures,
		const std::vector<ID3D12Resource*>& heightTextures) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE mBaseColorBufferGpuDescriptorsBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE mNormalBufferGpuDescriptorsBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE mHeightBufferGpuDescriptorsBegin;
};