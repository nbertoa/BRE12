#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

struct Material;

// Recorder that does color mapping + normal mapping + height mapping
class ColorHeightCmdListRecorder : public GeometryPassCmdListRecorder {
public:
	ColorHeightCmdListRecorder();
	~ColorHeightCmdListRecorder() = default;
	ColorHeightCmdListRecorder(const ColorHeightCmdListRecorder&) = delete;
	const ColorHeightCmdListRecorder& operator=(const ColorHeightCmdListRecorder&) = delete;
	ColorHeightCmdListRecorder(ColorHeightCmdListRecorder&&) = default;
	ColorHeightCmdListRecorder& operator=(ColorHeightCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept;

	// Preconditions:
	// - "geometryDataVec" must not be nullptr
	// - "geometryDataCount" must be greater than zero
	// - "materials" must not be nullptr
	// - "normals" must not be nullptr
	// - "heights" must not be nullptr
	// - "numResources" must be greater than zero
	void Init(
		const GeometryData* geometryDataVec,
		const std::uint32_t geometryDataCount,
		const Material* materials,
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
	// - "normals" must not be nullptr
	// - "heights" must not be nullptr
	// - "dataCount" must be greater than zero
	void BuildBuffers(
		const Material* materials,
		ID3D12Resource** normals,
		ID3D12Resource** heights,
		const std::uint32_t dataCount) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE mNormalsBufferGpuDescBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE mHeightsBufferGpuDescBegin;
};