#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

struct Material;

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
	// - "geometryDataVec" must not be nullptr
	// - "geometryDataCount" must be greater than zero
	// - "materials" must not be nullptr
	// - "textures" must not be nullptr
	// - "normals" must not be nullptr
	// - "heights" must not be nullptr
	// - "numResources" must be greater than zero
	// - InitSharedPSOAndRootSignature() must be called first and once
	void Init(
		const GeometryData* geometryDataVec,
		const std::uint32_t geometryDataCount,
		const Material* materials,
		ID3D12Resource** textures,
		ID3D12Resource** normals,
		ID3D12Resource** heights,
		const std::uint32_t numResources) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

	bool IsDataValid() const noexcept final override;

private:
	// Preconditions:
	// - "materials" must not be nullptr
	// - "textures" must not be nullptr
	// - "normals" must not be nullptr
	// - "heights" must not be nullptr
	// - "dataCount" must be greater than zero
	void InitConstantBuffers(
		const Material* materials,
		ID3D12Resource** textures,
		ID3D12Resource** normals,
		ID3D12Resource** heights,
		const std::uint32_t dataCount) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE mBaseColorBufferGpuDescriptorsBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE mNormalBufferGpuDescriptorsBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE mHeightBufferGpuDescriptorsBegin;
};