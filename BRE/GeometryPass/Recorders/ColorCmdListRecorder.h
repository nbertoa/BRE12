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

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept;

	// Preconditions:
	// - "geometryDataVec" must not be nullptr
	// - "geometryDataCount" must be greater than zero
	// - "materials" must not be nullptr
	// - "numMaterials" must be greater than zero
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
	void BuildBuffers(const Material* materials, const std::uint32_t numMaterials) noexcept;
};