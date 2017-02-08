#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

struct Material;

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
	// - "geometryDataVec" must not be nullptr
	// - "geometryDataCount" must be greater than zero
	// - "materials" must not be nullptr
	// - "numMaterials" must be greater than zero
	// - InitSharedPSOAndRootSignature() must be called first and once
	void Init(
		const GeometryData* geometryDataVec,
		const std::uint32_t geometryDataCount,
		const Material* materials,
		const std::uint32_t numMaterials) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

private:
	// Preconditions:
	// - "materials" must not be nullptr
	// - "numMaterials" must be greater than zero
	void InitConstantBuffers(const Material* materials, const std::uint32_t numMaterials) noexcept;
};